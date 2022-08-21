#include "image_to_ascii.h"
#include "CImg.h"
#include <array>
#include <climits>
#include <utility>
#include <sstream>

using namespace cimg_library;

size_t blur_d;
ssize_t color_off;

bool pix_sym_map::add(std::string symbol, std::vector<unsigned char> pixels) {
  //std::cout << "expected: " << pix_maps_size << " got: " << pixels.size() << std::endl;
  if (pixels.size() != pix_maps_size)
    return false;
  syms.push_back(symbol);
  pix_maps.push_back(pixels);
  return true;
};

const std::string pix_sym_map::find_best(gray_img& img, size_t x_off, size_t y_off) {
  size_t i, min_index = 0;
  int min_error;

  if (size() == 0)
    throw std::runtime_error("tried finding matching symbol from empty map");

  min_error = error_at(0, img, x_off, y_off);

  for (i = 1; i != pix_maps.size(); ++i) {
    if (error_at(i, img, x_off, y_off) < min_error) {
      min_index = i;
      min_error = error_at(i, img, x_off, y_off);
    }
  }

  return syms[min_index];
}

int pix_sym_map::error_at(size_t index, gray_img& img, size_t x_off, size_t y_off) {
  size_t i;
  int error = 0;

  for (i = 0; i != pix_maps_size; ++i)
    error += abs(pix_maps[index][i] - img[y_off + i / width][x_off + i % width] + color_off);

  return error;
}

void pix_sym_map::init_from_file(std::ifstream& in) {
  std::string line, sym = "";
  size_t i;
  std::vector<unsigned char> pix_map;

  getline(in, line);
  std::istringstream(line) >> width >> height;
  pix_maps_size = width * height;

  while (getline(in, sym)) {
    getline(in, line);
    pix_map.clear();

    for (i = 0; i != pix_maps_size; ++i) {
      if (i > line.size())
        throw std::runtime_error("missing pixels for symbol " + sym);
      pix_map.push_back(line[i]);
    }
    add(sym, std::vector<unsigned char>(pix_map));
  }
}

std::string convert_img_to_string(gray_img& img, pix_sym_map psm, size_t lines) {
  std::cout << "downscaling...";
  std::cout.flush();
  img.downscale((lines * psm.height) / (double) img.height);
  std::cout << " OK\nstretching...";
  std::cout.flush();
  img.stretch((img.width % psm.width == 0) ? 0 : psm.width - (img.width % psm.width), 0);
  std::cout << " OK\nblurring...";
  std::cout.flush();
  img.blur(blur_d);
  std::cout << " OK" << std::endl;

  //for (int y =0;y<img.height;y++) {for (int x =0;x<img.width;++x)if(img[y][x]==0)std::cout<<'_';else std::cout<<'o';std::cout<<std::endl;}
  size_t x, y;
  std::string res = "";

  std::cout << "generating result string...";
  std::cout.flush();
  for (y = 0; y < img.height; y += psm.height) {
    for (x = 0; x < img.width; x += psm.width) {
      //   std::cout << (int) img[x][y] << std::endl;
      res.append(psm.find_best(img, x, y));
    }
    res.append("\n");
  }
  std::cout << " OK" << std::endl;

  return res;
}

void gray_img::stretch(size_t dw, size_t dh) {
  size_t i;

  for (i = 0; i != height; ++i)
    (*img_data)[i].resize(width + dw, 0);

  for (i = 0; i != dh; ++i)
    (*img_data).push_back(std::vector<unsigned char>(width + dw, 0));

  width += dw;
  height += dh;
}

void gray_img::downscale(double scalar) {
  if (scalar > 1)
    throw std::runtime_error("tried upscaling image");
  if (scalar <= 0)
    throw std::runtime_error("invalid scalar");

  size_t x, y, _x, _y, new_width = std::round(scalar * width), new_height = std::round(scalar * height);
  auto new_img_data = std::make_unique<std::vector<std::vector<unsigned char>>>(new_height, std::vector<unsigned char>(new_width, 0));
  std::vector<std::vector<std::pair<int, int>>> acc(new_height, std::vector<std::pair<int, int>>(new_width));

  for (y = 0; y != height; ++y) {
    for (x = 0; x != width; ++x) {
      _x = std::min<size_t>(std::round(scalar * x), new_width - 1);
      _y = std::min<size_t>(std::round(scalar * y), new_height - 1);
      acc[_y][_x].first += (*img_data)[y][x];
      ++acc[_y][_x].second;
    }
  }

  width = new_width;
  height = new_height;

  for (y = 0; y != height; ++y)
    for (x = 0; x != width; ++x)
      (*new_img_data)[y][x] = (unsigned char) (acc[y][x].first / acc[y][x].second);

  img_data.swap(new_img_data);
}

/* TODO */
void gray_img::upscale(double scalar) {

}

bool gray_img::in_bounds(ssize_t x, ssize_t y) {
  return x >= 0 && x < width && y >= 0 && y < height;
}

void gray_img::blur(size_t reach) {
  if (reach == 0)
    return;
  auto new_img_data = std::make_unique<std::vector<std::vector<unsigned char>>>(height, std::vector<unsigned char>(width, 0));
  int acc = 0, amt = 0;
  ssize_t x = 0, y = 0, dx, dy;
  size_t x_start = 0, x_end = width - 1, x_dir = 1;

  for (dy = 0; dy != reach + 1; ++dy) {
    for (dx = 0; dx != reach; ++dx) {
      if (in_bounds(x + dx, y + dy)) {
        acc += (*img_data)[y + dy][x + dx];
        ++amt;
      }
    }
  }
  for (y = 0; y != height; ++y) {
    for (x = x_start; x != x_end + x_dir; x += x_dir) {
      for (dy = -reach; dy != reach + 1; ++dy) {
        if (in_bounds(x - x_dir * (reach + 1), y + dy)) {
          acc -= (*img_data)[y + dy][x - x_dir * (reach + 1)];
          --amt;
        }
        if (in_bounds(x + x_dir * reach, y + dy)) {
          acc += (*img_data)[y + dy][x + x_dir * reach];
          ++amt;
        }
      }
      (*new_img_data)[y][x] = (unsigned char) std::round(acc / amt);
    }

    /* repeat for last step in line */
    for (dy = -reach; dy != reach + 1; ++dy) {
      if (in_bounds(x - x_dir * (reach + 1), y + dy)) {
        acc -= (*img_data)[y + dy][x - x_dir * (reach + 1)];
        --amt;
      }
      if (in_bounds(x + x_dir * reach, y + dy)) {
        acc += (*img_data)[y + dy][x + x_dir * reach];
        ++amt;
      }
    }

    for (dx = -reach; dx != reach + 1; ++dx) {
      if (in_bounds(x + dx, y - reach)) {
        acc -= (*img_data)[y - reach][x + dx];
        --amt;
      }
      if (in_bounds(x + dx, y + reach + 1)) {
        acc += (*img_data)[y + reach + 1][x + dx];
        ++amt;
      }
    }
    x_dir *= -1;
    std::swap(x_start, x_end);
  }
  img_data.swap(new_img_data);
}

gray_img::gray_img(const std::string filename) {
  size_t x, y;
  CImg<unsigned char> image(filename.c_str());
  height = image.height();
  width = image.width();
  img_data = std::make_unique<std::vector<std::vector<unsigned char>>>(std::vector<std::vector<unsigned char>>(height, std::vector<unsigned char>(width, 0)));

  //(*img_data)[y][x] = image(112, 54, 0, 1);

  for (y = 0; y != height; ++y)
    for (x = 0; x != width; ++x) {
      //std::cout << x << ", " << y << std::endl;
      (*img_data)[y][x] = 0.3 * image(x, y, 0, 0) + 0.6 * image(x, y, 0, 1) + 0.1 * image(x, y, 0, 2);
    }
}

pix_sym_map::pix_sym_map(std::ifstream& bdf) {
  int chars = 0, i, j, k, fbb_w = 0, fbb_h = 0, fbb_x_off = 0, fbb_y_off = 0;
  std::string line, skip;

  while (getline(bdf, line)) {
    if (!line.rfind("FONTBOUNDINGBOX", 0)) {
      std::istringstream(line) >> skip >> fbb_w >> fbb_h >> fbb_x_off >> fbb_y_off;
      width = fbb_w - 1;
      height = fbb_h - 1;
    }
    if (!line.rfind("CHARS ", 0)) {
      std::istringstream(line) >> skip >> chars;
      break;
    }
  }
  pix_maps_size = width * height;
  if (!chars)
    throw std::runtime_error("invalid CHARS member in bdf file");
  std::vector<unsigned char> bitmap(width * height, 0);
  for (i = 0; i != chars; ++i) {
    bitmap.clear();
    int encoding = -1, x_off = 0, y_off = 0, w = 0, h = 0;
    while (getline(bdf, line)) {
      if (!line.rfind("STARTCHAR", 0))
        break;
    }
    while (getline(bdf, line)) {
      if (!line.rfind("BBX", 0))
        std::istringstream(line) >> skip >> w >> h >> x_off >> y_off;
      if (!line.rfind("ENCODING", 0))
        std::istringstream(line) >> skip >> encoding;
      if (!line.rfind("BITMAP", 0)) {
        for (j = 0; j < ((ssize_t) height) - h - y_off + fbb_y_off; ++j)
          for (k = 0; k != width; ++k)
            bitmap.push_back(0);
        while (getline(bdf, line)) {
          if (!line.rfind("ENDCHAR", 0))
            break;
          for (j = 0; j < x_off - fbb_x_off; ++j)
            bitmap.push_back(0);
          for (; j < std::min((ssize_t) w + x_off - fbb_x_off, ((ssize_t) width)); ++j) {
            size_t pos = std::min(j - x_off + fbb_x_off, j);
            bitmap.push_back((std::stoul(std::string("0x") += (line[std::floor(pos / 4)]), nullptr, 16) & (1 << (3 - pos % 4))) ? (UCHAR_MAX >> 1) : 0);
          }
          for (; j < width; ++j)
            bitmap.push_back(0);
        }
        for (j = std::floor(bitmap.size() / width); j < height; ++j)
          for (k = 0; k != width; ++k)
            bitmap.push_back(0);
        std::string sym = "";
        if (encoding == -1)
          throw std::runtime_error("no encoding provided for char");
        while (encoding != 0) {
          unsigned char c = encoding & UCHAR_MAX;
          encoding >>= sizeof(unsigned char) * 8;
          sym += c;
        }
        std::reverse(sym.begin(), sym.end());
        add(sym, bitmap);
        break;
      }
    }
    //std::cout << "processed " << (i + 1) << "/" << chars << " symbol(s)" << std::endl;
  }
}

int main(int argc, char **argv) {
  std::cout << "reading image file...";
  std::cout.flush();
  gray_img img(argv[2]);
  std::cout << " OK\nreading mapping file...";
  std::cout.flush();
  std::ifstream test(argv[1]);
  pix_sym_map psm(test);
  if (argc > 4)
    std::istringstream(argv[4]) >> blur_d;
  else
    blur_d = 0;
  if (argc > 5)
    std::istringstream(argv[5]) >> color_off;
  else
    color_off = 0;
  std::cout << " OK\n\nIMAGE CONVERSION STARTED!" << std::endl;
  auto res = convert_img_to_string(img, psm, std::atoi(argv[3]));
  std::cout << std::endl << res << std::endl << "done." << std::endl;

  return 0;
}

