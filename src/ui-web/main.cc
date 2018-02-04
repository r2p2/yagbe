#include <crow_all.h>

#include "../gb/gb.hpp"

std::string screen_to_ppm(GR::screen_t screen, int width)
{
  // "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
  //   "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n" 
  std::string image =
    "<svg xmlns=\"http://www.w3.org/2000/svg\"" \
    " xmlns:xlink=\"http://www.w3.org/1999/xlink\"" \
    " version=\"1.1\" baseProfile=\"full\"" \
    " width=\"160px\" height=\"144px\">";

  for (int i = 0; i < screen.size(); ++i) {
    if (screen[i] == 0)
      continue;

    auto const x = i % width;
    auto const y = i / width;
    image += "<rect x=\"" + std::to_string(x) + "\" y=\"" + std::to_string(y) + "\" width=\"1px\" height=\"1px\"/>";
  }

  // "<circle cx=\"50\" cy=\"50\" r=\"40\" stroke=\"black\" stroke-width=\"3\" fill=\"red\" />";
  image +=
    "Sorry, your browser does not support inline SVG."  \
    "</svg>";
  return image;
}

int main(int argc, char** argv)
{
  if (argc < 2)
    return 1;

  std::ifstream s_cart(
    argv[1],
    std::ios::in | std::ios::binary);

  GB::cartridge_t cart(
    (std::istreambuf_iterator<char>(s_cart)),
    std::istreambuf_iterator<char>());

  GB gb;
  gb.insert_rom(cart);
  gb.power_on();

  crow::SimpleApp app;
  std::string const page =
    "<html>" \
    "<head><script type=\"text/javascript\" src=\"http://ajax.googleapis.com/ajax/libs/jquery/1.3/jquery.min.js\"></script>" \
    "</head>" \
    "<body>" \
    "<div id=\"image_container\"><img id=\"screen\" width=\"160\" height=\"144\" style=\"border:1px solid black\" ></img></div>" \
    "</body>" \
    "<script>" \
    "function x(index) { svg = $(\"#screen\"); svg.load(\"/screen.svg?i=\" + index, function () { x(index+1) }); } /*x(1)*/;" \
    "</script>" \
    "</html>";

  CROW_ROUTE(app, "/")([&](){
      return page;
    });
  CROW_ROUTE(app, "/screen.svg")([&](){
      do {
        gb.tick();
      }
      while(not gb.is_v_blank_completed());

      auto const screen = gb.screen();

      std::string data;
      for (size_t i = 0; i < screen.size(); ++i) {
        data += std::to_string(screen[i]);
        if ((i % gb.screen_width()) == 0)
          data += "<br>";
      }
      auto response = crow::response(screen_to_ppm(screen, gb.screen_width()));
      response.set_header("Content-Type", "image/svg+xml");
      return response;
    });

  app.port(8080).run();
}
