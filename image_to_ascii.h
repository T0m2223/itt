#ifndef IMAGE_TO_STRING_H
#define IMAGE_TO_STRING_H

#include <cstddef>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>


class gray_img;


class pix_sym_map {
  friend std::string convert_img_to_string(gray_img& img, pix_sym_map psm, size_t lines);
  size_t pix_maps_size, width, height;
  std::vector<std::vector<unsigned char>> pix_maps;
  std::vector<std::string> syms;

public:
  pix_sym_map(std::ifstream& in);
  const std::string find_best(gray_img& img, size_t x_off, size_t y_off);
  bool add(std::string symbol, std::vector<unsigned char> pixels);
  size_t size() { return pix_maps.size(); };

private:
  int error_at(size_t index, gray_img& img, size_t x_off, size_t y_off);
  void init_from_file(std::ifstream& in);
};


class gray_img {
  friend std::string convert_img_to_string(gray_img& img, pix_sym_map psm, size_t lines);
  std::unique_ptr<std::vector<std::vector<unsigned char>>> img_data;
  size_t width, height;

public:
  size_t get_width() { return width; };
  gray_img(const std::string filename);
  gray_img(std::ifstream& bdf);
  std::vector<unsigned char>& operator[](size_t line) { return (*img_data)[line]; };
  void downscale(double scalar);
  void upscale(double scalar);
  void stretch(size_t dw, size_t dh);
  void blur(size_t reach);
private:
  bool in_bounds(ssize_t x, ssize_t y);
};


std::string convert_img_to_string(gray_img& img, pix_sym_map psm, size_t lines);

#endif
