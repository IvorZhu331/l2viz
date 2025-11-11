#include "common.hpp"
#include "book.hpp"

#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include <SDL.h>

#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include <cmath>

static constexpr int  LEVELS = 12;
static constexpr int  FPS = 60;
static constexpr uint32_t MIN_SIZE = 1;

int main() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::fprintf(stderr,"SDL init failed: %s\n", SDL_GetError()); 
        return 1;
    }
    
    SDL_Window* win = SDL_CreateWindow("L2 Visualizer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1200, 700, SDL_WINDOW_RESIZABLE);
    if (!win) { 
        std::fprintf(stderr,"SDL_CreateWindow failed: %s\n", SDL_GetError()); 
        return 1; 
    }

    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!ren) { 
        std::fprintf(stderr,"SDL_CreateRenderer failed: %s\n", SDL_GetError()); 
        return 1; 
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(win, ren);
    ImGui_ImplSDLRenderer2_Init(ren);

    Book book;
    std::mutex mtx;
    std::atomic<bool> running{true};
    std::atomic<bool> eof{false};

    std::thread reader([&](){
        std::vector<unsigned char> buf;
        buf.reserve(1<<20);
        while (true) {
            unsigned char tmp[1<<16];
            size_t n = std::fread(tmp,1,sizeof(tmp), stdin);
            if (n == 0) { 
                eof.store(true); 
                break; 
            }
            buf.insert(buf.end(), tmp, tmp+n);

            const size_t rec = sizeof(L2Event);
            size_t m = (buf.size()/rec)*rec;
            for (size_t off=0; off+rec<=m; off+=rec) {
                L2Event ev;
                std::memcpy(&ev, buf.data()+off, rec);
                std::lock_guard<std::mutex> lk(mtx);
                book.apply(ev.ts_ns, ev.price, ev.size, ev.side, ev.rtype);
            }
            if (m) buf.erase(buf.begin(), buf.begin()+m);
        }
    });

    const double frame_ms = 1000.0 / std::max(1, FPS);
    
    while (running.load()) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT) running.store(false);
            if (e.type == SDL_WINDOWEVENT && e.window.event==SDL_WINDOWEVENT_CLOSE) 
                running.store(false);
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Status
        ImGui::Begin("Status");
        ImGui::Text("EOF: %s", eof.load() ? "yes" : "no");
        ImGui::End();

        // Top-of-Book
        ImGui::Begin("Top-of-Book");
        {
            std::lock_guard<std::mutex> lk(mtx);
            ImGui::Text("Best Bid: %s", book.best_bid ? std::to_string(*book.best_bid).c_str() : "-");
            ImGui::Text("Best Ask: %s", book.best_ask ? std::to_string(*book.best_ask).c_str() : "-");
            if (book.best_bid && book.best_ask) {
                ImGui::Text("Spread: %.6f", *book.best_ask - *book.best_bid);
            }
        }
        ImGui::End();

        // Price Ladder
        ImGui::Begin("Price Ladder");
        {
            std::lock_guard<std::mutex> lk(mtx);
            auto bids = book.top_bids(LEVELS, MIN_SIZE);
            auto asks = book.top_asks(LEVELS, MIN_SIZE);
            ImGui::Text("BIDS (left) | ASKS (right)"); 
            ImGui::Separator();

            float max_sz = 1.f;
            for (auto& x : bids) max_sz = std::max(max_sz, (float)x.second);
            for (auto& x : asks) max_sz = std::max(max_sz, (float)x.second);

            const float row_h = ImGui::GetTextLineHeightWithSpacing() * 1.2f;
            for (int i=0; i<std::max(bids.size(), asks.size()); ++i) {
                if (i < (int)bids.size()) {
                    auto [p,s] = bids[i];
                    ImGui::Text("%10.4f", p); 
                    ImGui::SameLine(110);
                    ImGui::ProgressBar(s/max_sz, ImVec2(150,row_h)); 
                    ImGui::SameLine(270);
                    ImGui::Text("%u", s);
                } else {
                    ImGui::Dummy(ImVec2(300,row_h));
                }

                ImGui::SameLine(360);

                if (i < (int)asks.size()) {
                    auto [p,s] = asks[i];
                    ImGui::Text("%u", s); 
                    ImGui::SameLine(430);
                    ImGui::ProgressBar(s/max_sz, ImVec2(150,row_h)); 
                    ImGui::SameLine(600);
                    ImGui::Text("%10.4f", p);
                }
            }
        }
        ImGui::End();

        ImGui::Render();
        SDL_SetRenderDrawColor(ren, 25,26,31,255);
        SDL_RenderClear(ren);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), ren);
        SDL_RenderPresent(ren);

        SDL_Delay((Uint32)frame_ms);
    }

    reader.join();
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
