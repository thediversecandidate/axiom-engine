// axiom_triangle — Stage 1 window path (ROADMAP Stage 1 deliverables). Opens an SDL3 window and draws the
// Stage 1 triangle (the scene of tests/golden/triangle.json) every frame through Swapchain + TrianglePipeline,
// recreating the swapchain on resize or when it is out of date.
//
// usage: axiom_triangle [--frames N] [--validation]
//   --frames N     exit after N presented frames (CI smoke test with SDL_VIDEO_DRIVER=offscreen)
//   --validation   enable VK_LAYER_KHRONOS_validation; any validation error makes the exit code 1
// Esc or closing the window quits.

#include "axiom/renderer/triangle_pipeline.hpp"
#include "axiom/renderer_present/swapchain.hpp"
#include "axiom/renderer_vk/command_context.hpp"
#include "axiom/renderer_vk/gpu_allocator.hpp"
#include "axiom/renderer_vk/host_buffer.hpp"
#include "axiom/renderer_vk/validation.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>

using namespace axiom;
using namespace axiom::renderer;

namespace {

// Same vertices and camera as tests/golden/triangle.json (CONVENTIONS §4 projection, fovY 60°, eye (0,0,2)).
constexpr std::array<TriangleVertex, 3> kTriangle{{
    {{-0.6f, -0.6f, 0.0f}, {1, 0, 0}},
    {{0.6f, -0.6f, 0.0f}, {0, 1, 0}},
    {{0.0f, 1.2f, -1.5f}, {0, 0, 1}},
}};
constexpr float kFocal = 1.7320508075688772f; // 1 / tan(30°)

void cameraMvp(float aspect, float (&mvp)[16]) {
  const float m[16] = {kFocal / aspect, 0, 0, 0, 0, -kFocal, 0, 0, 0, 0, 0, -1, 0, 0, 0.1f, 2};
  std::memcpy(mvp, m, sizeof(m));
}

int report(const char *what, const core::Error &e) {
  std::fprintf(stderr, "axiom_triangle: %s: %.*s\n", what, static_cast<int>(e.message.size()), e.message.data());
  return 1;
}

int reportSdl(const char *what) {
  std::fprintf(stderr, "axiom_triangle: %s: %s\n", what, SDL_GetError());
  return 1;
}

struct Options {
  long frames = 0; // 0 = until quit
  bool validation = false;
};

bool parse(int argc, char **argv, Options &o) {
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--validation") == 0) {
      o.validation = true;
    } else if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
      char *end = nullptr;
      o.frames = std::strtol(argv[++i], &end, 10);
      if (*end != '\0' || o.frames <= 0) {
        return false;
      }
    } else {
      return false;
    }
  }
  return true;
}

// Runs the frame loop; everything Vulkan is destroyed before this returns (the surface is owned by the caller).
int run(const Options &opt, SDL_Window *window, const VulkanInstance &instance, VkSurfaceKHR surface) {
  auto device = VulkanDevice::create(instance, DeviceDesc{std::nullopt, surface});
  if (!device) {
    return report("device", device.error());
  }
  auto allocator = GpuAllocator::create(instance, *device);
  if (!allocator) {
    return report("allocator", allocator.error());
  }
  int w = 0, h = 0;
  SDL_GetWindowSizeInPixels(window, &w, &h);
  auto swapchain = Swapchain::create(
      *device, SwapchainDesc{surface, static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h), 0});
  if (!swapchain) {
    return report("swapchain", swapchain.error());
  }
  auto pipeline = TrianglePipeline::create(*device, swapchain->viewFormat());
  if (!pipeline) {
    return report("pipeline", pipeline.error());
  }
  auto vertices =
      HostBuffer::create(*allocator, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, std::as_bytes(std::span{kTriangle}));
  if (!vertices) {
    return report("vertex buffer", vertices.error());
  }
  auto commands = CommandContext::create(*device);
  if (!commands) {
    return report("commands", commands.error());
  }

  long presented = 0;
  bool resized = false;
  for (bool running = true; running && (opt.frames == 0 || presented < opt.frames);) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
        running = false;
      } else if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
        resized = true;
      }
    }
    SDL_GetWindowSizeInPixels(window, &w, &h);
    if (w == 0 || h == 0) { // minimized: nothing to present until restored
      SDL_WaitEvent(nullptr);
      continue;
    }
    if (resized) {
      if (auto r = swapchain->recreate(static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h)); !r) {
        return report("swapchain recreate", r.error());
      }
      resized = false;
    }
    auto frame = swapchain->acquire();
    if (!frame) {
      return report("acquire", frame.error());
    }
    if (!frame->has_value()) {
      resized = true; // out of date
      continue;
    }
    auto cmd = commands->begin();
    if (!cmd) {
      return report("begin", cmd.error());
    }
    const VkExtent2D extent = swapchain->extent();
    float mvp[16];
    cameraMvp(static_cast<float>(extent.width) / static_cast<float>(extent.height), mvp);
    swapchain->recordBegin(*cmd, **frame, {0.0f, 0.0f, 0.0f, 1.0f});
    pipeline->recordBind(*cmd, extent.width, extent.height, VK_FRONT_FACE_COUNTER_CLOCKWISE, mvp);
    const VkBuffer vb = vertices->handle();
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(*cmd, 0, 1, &vb, &offset);
    vkCmdDraw(*cmd, 3, 1, 0, 0);
    swapchain->recordEnd(*cmd, **frame);
    if (auto s = commands->submitAndWait((*frame)->acquired, (*frame)->renderFinished); !s) {
      return report("submit", s.error());
    }
    auto ok = swapchain->present(**frame);
    if (!ok) {
      return report("present", ok.error());
    }
    resized = resized || !*ok;
    ++presented;
  }
  std::printf("axiom_triangle: presented %ld frames on driver %d\n", presented, static_cast<int>(device->driverId()));
  return 0;
}

} // namespace

int main(int argc, char **argv) {
  Options opt;
  if (!parse(argc, argv, opt)) {
    std::fprintf(stderr, "usage: axiom_triangle [--frames N] [--validation]\n");
    return 2;
  }
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    return reportSdl("SDL_Init");
  }
  SDL_Window *window = SDL_CreateWindow("Axiom — Stage 1 triangle", 800, 800, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
  if (window == nullptr) {
    SDL_Quit();
    return reportSdl("SDL_CreateWindow");
  }
  Uint32 count = 0;
  const char *const *sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&count);
  ValidationSink sink;
  int status = 1;
  {
    auto instance =
        VulkanInstance::create(InstanceDesc{"axiom_triangle", opt.validation, opt.validation ? &sink : nullptr,
                                            std::span<const char *const>(sdlExtensions, count)});
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (!instance) {
      status = report("instance", instance.error());
    } else if (!SDL_Vulkan_CreateSurface(window, instance->handle(), nullptr, &surface)) {
      status = reportSdl("SDL_Vulkan_CreateSurface");
    } else {
      status = run(opt, window, *instance, surface);
      SDL_Vulkan_DestroySurface(instance->handle(), surface, nullptr);
    }
  }
  SDL_DestroyWindow(window);
  SDL_Quit();
  if (opt.validation && sink.errors.load() != 0) {
    std::fprintf(stderr, "axiom_triangle: %u validation error(s); first: %s\n", sink.errors.load(), sink.firstError);
    return 1;
  }
  return status;
}
