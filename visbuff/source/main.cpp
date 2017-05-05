#include <EASTL/unique_ptr.h>
#include <EASTL/utility.h>
#include <base_system.h>
#include <visbuff_scene.h>

int main() {

  vks::Init();
  eastl::unique_ptr<vks::VisbuffScene> scene =
      eastl::make_unique<vks::VisbuffScene>();
  vks::Run(scene.get());
  vks::Shutdown();

  return 0;
}
