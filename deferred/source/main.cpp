#include <EASTL/unique_ptr.h>
#include <EASTL/utility.h>
#include <base_system.h>
#include <deferred_scene.h>

int main() {

  vks::Init();
  eastl::unique_ptr<vks::DeferredScene> scene =
      eastl::make_unique<vks::DeferredScene>();
  vks::Run(scene.get());
  vks::Shutdown();

  return 0;
}
