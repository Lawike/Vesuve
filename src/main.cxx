// #include "VesuveApp.hpp"
#include "VkEngine.hpp"


int main()
{
  VkEngine renderEngine;

  renderEngine.init();

  renderEngine.run();

  renderEngine.cleanup();

  return 0;
  /*VesuveApp app;

  try
  {
    app.run();
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
  */
}
