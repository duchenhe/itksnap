#include "ColorMap.h"

#include <iostream>
#include <string>

class TestColorMapPresetNameSource : public AbstractColorMapPresetNameSource
{
public:
  std::string GetPresetName(ColorMap::SystemPreset preset, bool) override
  {
    return "Color Map " + std::to_string(static_cast<int>(preset));
  }
};

namespace
{

struct ExpectedPreset
{
  ColorMap::SystemPreset preset;
  unsigned char first[3];
  unsigned char middle[3];
  unsigned char last[3];
};

bool CheckColor(const char *location,
                const ColorMap::RGBAType &actual,
                const unsigned char expected[3])
{
  if(actual[0] == expected[0] && actual[1] == expected[1] &&
     actual[2] == expected[2] && actual[3] == 0xff)
    return true;

  std::cerr << location << " mismatch: got ("
            << static_cast<int>(actual[0]) << ", "
            << static_cast<int>(actual[1]) << ", "
            << static_cast<int>(actual[2]) << ", "
            << static_cast<int>(actual[3]) << ")" << std::endl;
  return false;
}

} // namespace

int main()
{
  TestColorMapPresetNameSource nameSource;
  ColorMap::SetColorMapPresetNameSource(&nameSource);

  const ExpectedPreset expectedPresets[] = {
    {ColorMap::COLORMAP_INFERNO, {0x00, 0x00, 0x04}, {0xbc, 0x37, 0x54}, {0xfc, 0xff, 0xa4}},
    {ColorMap::COLORMAP_VIRIDIS, {0x44, 0x01, 0x54}, {0x21, 0x91, 0x8c}, {0xfd, 0xe7, 0x25}},
    {ColorMap::COLORMAP_PLASMA,  {0x0d, 0x08, 0x87}, {0xcc, 0x47, 0x78}, {0xf0, 0xf9, 0x21}},
    {ColorMap::COLORMAP_MAGMA,   {0x00, 0x00, 0x04}, {0xb7, 0x37, 0x79}, {0xfc, 0xfd, 0xbf}},
    {ColorMap::COLORMAP_CIVIDIS, {0x00, 0x22, 0x4e}, {0x7d, 0x7c, 0x78}, {0xfe, 0xe8, 0x38}},
    {ColorMap::COLORMAP_TURBO,   {0x30, 0x12, 0x3b}, {0xa4, 0xfc, 0x3c}, {0x7a, 0x04, 0x03}}
  };

  ColorMap::Pointer colorMap = ColorMap::New();
  for(const ExpectedPreset &expected : expectedPresets)
    {
    colorMap->SetToSystemPreset(expected.preset);
    if(colorMap->GetSystemPreset() != expected.preset)
      {
      std::cerr << "Preset identity was not retained" << std::endl;
      return 1;
      }
    if(colorMap->GetNumberOfCMPoints() != 17)
      {
      std::cerr << "Expected 17 control points, got "
                << colorMap->GetNumberOfCMPoints() << std::endl;
      return 1;
      }
    if(!CheckColor("first", colorMap->MapIndexToRGBA(0.0), expected.first) ||
       !CheckColor("middle", colorMap->MapIndexToRGBA(0.5), expected.middle) ||
       !CheckColor("last", colorMap->MapIndexToRGBA(1.0), expected.last))
      return 1;
    }

  return 0;
}
