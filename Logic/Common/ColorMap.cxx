/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: ColorMap.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/18 11:25:44 $
  Version:   $Revision: 1.12 $
  Copyright (c) 2007 Paul A. Yushkevich
  
  This file is part of ITK-SNAP 

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information. 

=========================================================================*/
#include "ColorMap.h"
#include "IRISException.h"
#include <algorithm>
#include <cstdio>

namespace
{

struct PresetColorPoint
{
  double index;
  unsigned char red;
  unsigned char green;
  unsigned char blue;
};

// These 17-point anchors are sampled at i/16 from Matplotlib's 256-entry
// listed colormaps and rounded to 8-bit RGB.  Keeping a modest number of
// anchors gives a close visual match without making the presets expensive to
// edit or serialize.  Source: matplotlib/lib/matplotlib/_cm_listed.py.
constexpr PresetColorPoint INFERNO_POINTS[] = {
  {0.0000, 0x00, 0x00, 0x04}, {0.0625, 0x0b, 0x07, 0x24},
  {0.1250, 0x21, 0x0c, 0x4a}, {0.1875, 0x3d, 0x09, 0x65},
  {0.2500, 0x57, 0x10, 0x6e}, {0.3125, 0x71, 0x19, 0x6e},
  {0.3750, 0x8a, 0x22, 0x6a}, {0.4375, 0xa3, 0x2c, 0x61},
  {0.5000, 0xbc, 0x37, 0x54}, {0.5625, 0xd2, 0x46, 0x44},
  {0.6250, 0xe4, 0x5a, 0x31}, {0.6875, 0xf1, 0x73, 0x1d},
  {0.7500, 0xf9, 0x8e, 0x09}, {0.8125, 0xfc, 0xac, 0x11},
  {0.8750, 0xf9, 0xcb, 0x35}, {0.9375, 0xf2, 0xea, 0x69},
  {1.0000, 0xfc, 0xff, 0xa4}
};

constexpr PresetColorPoint VIRIDIS_POINTS[] = {
  {0.0000, 0x44, 0x01, 0x54}, {0.0625, 0x48, 0x18, 0x6a},
  {0.1250, 0x47, 0x2d, 0x7b}, {0.1875, 0x42, 0x40, 0x86},
  {0.2500, 0x3b, 0x52, 0x8b}, {0.3125, 0x33, 0x63, 0x8d},
  {0.3750, 0x2c, 0x72, 0x8e}, {0.4375, 0x26, 0x82, 0x8e},
  {0.5000, 0x21, 0x91, 0x8c}, {0.5625, 0x1f, 0xa0, 0x88},
  {0.6250, 0x28, 0xae, 0x80}, {0.6875, 0x3f, 0xbc, 0x73},
  {0.7500, 0x5e, 0xc9, 0x62}, {0.8125, 0x84, 0xd4, 0x4b},
  {0.8750, 0xad, 0xdc, 0x30}, {0.9375, 0xd8, 0xe2, 0x19},
  {1.0000, 0xfd, 0xe7, 0x25}
};

constexpr PresetColorPoint PLASMA_POINTS[] = {
  {0.0000, 0x0d, 0x08, 0x87}, {0.0625, 0x31, 0x05, 0x97},
  {0.1250, 0x4c, 0x02, 0xa1}, {0.1875, 0x66, 0x00, 0xa7},
  {0.2500, 0x7e, 0x03, 0xa8}, {0.3125, 0x95, 0x11, 0xa1},
  {0.3750, 0xaa, 0x23, 0x95}, {0.4375, 0xbc, 0x35, 0x87},
  {0.5000, 0xcc, 0x47, 0x78}, {0.5625, 0xda, 0x5a, 0x6a},
  {0.6250, 0xe6, 0x6c, 0x5c}, {0.6875, 0xf0, 0x80, 0x4e},
  {0.7500, 0xf8, 0x95, 0x40}, {0.8125, 0xfd, 0xac, 0x33},
  {0.8750, 0xfd, 0xc5, 0x27}, {0.9375, 0xf8, 0xdf, 0x25},
  {1.0000, 0xf0, 0xf9, 0x21}
};

constexpr PresetColorPoint MAGMA_POINTS[] = {
  {0.0000, 0x00, 0x00, 0x04}, {0.0625, 0x0a, 0x08, 0x22},
  {0.1250, 0x1d, 0x11, 0x47}, {0.1875, 0x36, 0x10, 0x6b},
  {0.2500, 0x51, 0x12, 0x7c}, {0.3125, 0x6a, 0x1c, 0x81},
  {0.3750, 0x83, 0x26, 0x81}, {0.4375, 0x9c, 0x2e, 0x7f},
  {0.5000, 0xb7, 0x37, 0x79}, {0.5625, 0xd0, 0x41, 0x6f},
  {0.6250, 0xe7, 0x52, 0x63}, {0.6875, 0xf5, 0x6b, 0x5c},
  {0.7500, 0xfc, 0x89, 0x61}, {0.8125, 0xfe, 0xa7, 0x72},
  {0.8750, 0xfe, 0xc4, 0x88}, {0.9375, 0xfd, 0xe2, 0xa3},
  {1.0000, 0xfc, 0xfd, 0xbf}
};

constexpr PresetColorPoint CIVIDIS_POINTS[] = {
  {0.0000, 0x00, 0x22, 0x4e}, {0.0625, 0x00, 0x2e, 0x6a},
  {0.1250, 0x1a, 0x38, 0x6f}, {0.1875, 0x32, 0x43, 0x6d},
  {0.2500, 0x43, 0x4e, 0x6c}, {0.3125, 0x53, 0x5a, 0x6d},
  {0.3750, 0x61, 0x65, 0x6f}, {0.4375, 0x6f, 0x70, 0x73},
  {0.5000, 0x7d, 0x7c, 0x78}, {0.5625, 0x8c, 0x88, 0x78},
  {0.6250, 0x9b, 0x94, 0x76}, {0.6875, 0xab, 0xa0, 0x72},
  {0.7500, 0xbc, 0xae, 0x6c}, {0.8125, 0xcd, 0xbb, 0x63},
  {0.8750, 0xde, 0xc9, 0x58}, {0.9375, 0xf0, 0xd8, 0x46},
  {1.0000, 0xfe, 0xe8, 0x38}
};

constexpr PresetColorPoint TURBO_POINTS[] = {
  {0.0000, 0x30, 0x12, 0x3b}, {0.0625, 0x40, 0x40, 0xa2},
  {0.1250, 0x46, 0x6b, 0xe3}, {0.1875, 0x42, 0x94, 0xff},
  {0.2500, 0x28, 0xbc, 0xeb}, {0.3125, 0x18, 0xdd, 0xc2},
  {0.3750, 0x32, 0xf2, 0x98}, {0.4375, 0x6d, 0xfe, 0x62},
  {0.5000, 0xa4, 0xfc, 0x3c}, {0.5625, 0xcd, 0xec, 0x34},
  {0.6250, 0xee, 0xcf, 0x3a}, {0.6875, 0xfd, 0xac, 0x34},
  {0.7500, 0xfb, 0x7e, 0x21}, {0.8125, 0xeb, 0x50, 0x0e},
  {0.8750, 0xd0, 0x2f, 0x05}, {0.9375, 0xa9, 0x16, 0x01},
  {1.0000, 0x7a, 0x04, 0x03}
};

} // namespace

bool
ColorMap::CMPoint
::operator==(const ColorMap::CMPoint& rhs) const
{
  if (m_Index != rhs.m_Index)
    return false;
  if (m_Type != rhs.m_Type)
    return false;
  for (int i = 0; i < 2; ++i)
    {
    for (int j = 0; j < 4; ++j)
      {
      if (m_RGBA[i][j] != rhs.m_RGBA[i][j])
        return false;
      }
    }
  return true;
}

bool
ColorMap::CMPoint
::operator!=(const ColorMap::CMPoint& rhs) const
{
  return !(*this == rhs);
}

ColorMap::CMPoint
::CMPoint()
{
  m_Index = 0.0;
  m_Type = CONTINUOUS;

  m_RGBA[0][0] = 0;  m_RGBA[1][0] = 0;
  m_RGBA[0][1] = 0;  m_RGBA[1][1] = 0;
  m_RGBA[0][2] = 0;  m_RGBA[1][2] = 0;
  m_RGBA[0][3] = 0;  m_RGBA[1][3] = 0;
  }


// Continuous point with linear alpha
ColorMap::CMPoint
::CMPoint(double j, EltType r, EltType g, EltType b)
{
  m_Index = j;
  m_Type = (j == 0.0 || j == 1.0) ? DISCONTINUOUS : CONTINUOUS;
  EltType a = (EltType)(j * 255);

  m_RGBA[0][0] = r;  m_RGBA[1][0] = r;
  m_RGBA[0][1] = g;  m_RGBA[1][1] = g;
  m_RGBA[0][2] = b;  m_RGBA[1][2] = b;
  m_RGBA[0][3] = a;  m_RGBA[1][3] = a;
}

// Continuous point
ColorMap::CMPoint
::CMPoint(double j, EltType r, EltType g, EltType b, EltType a)
{
  m_Index = j;
  m_Type = CONTINUOUS;

  m_RGBA[0][0] = r;  m_RGBA[1][0] = r;
  m_RGBA[0][1] = g;  m_RGBA[1][1] = g;
  m_RGBA[0][2] = b;  m_RGBA[1][2] = b;
  m_RGBA[0][3] = a;  m_RGBA[1][3] = a;
}

// CMPoint with alpha discontinuity
ColorMap::CMPoint
::CMPoint(double j, EltType r, EltType g, EltType b, EltType a1, EltType a2)
{
  m_Index = j;
  m_Type = DISCONTINUOUS;

  m_RGBA[0][0] = r;  m_RGBA[1][0] = r;
  m_RGBA[0][1] = g;  m_RGBA[1][1] = g;
  m_RGBA[0][2] = b;  m_RGBA[1][2] = b;
  m_RGBA[0][3] = a1; m_RGBA[1][3] = a2;
}

// CMPoint with full discontinuity
ColorMap::CMPoint
::CMPoint(double j, 
    EltType r1, EltType g1, EltType b1, EltType a1,
    EltType r2, EltType g2, EltType b2, EltType a2)
{
  m_Index = j;
  m_Type = DISCONTINUOUS;

  m_RGBA[0][0] = r1; m_RGBA[1][0] = r2;
  m_RGBA[0][1] = g1; m_RGBA[1][1] = g2;
  m_RGBA[0][2] = b1; m_RGBA[1][2] = b2;
  m_RGBA[0][3] = a1; m_RGBA[1][3] = a2;
}

// CMPoint copy constructor
ColorMap::CMPoint
::CMPoint(const CMPoint& rhs)
{
  m_Index = rhs.m_Index;
  m_Type = rhs.m_Type;
  for (int i = 0; i < 2; ++i)
    {
    for (int j = 0; j < 4; ++j)
      {
      m_RGBA[i][j] = rhs.m_RGBA[i][j];
      }
    }
}

bool operator < (const ColorMap::CMPoint &p1, const ColorMap::CMPoint &p2)
{
  return p1.m_Index < p2.m_Index;
}

ColorMap
::ColorMap()
{
  SetToSystemPreset(COLORMAP_GREY);

  // Set up the enum map
  if(!m_ColorMapPresetEnumMap.Size())
    {
    for(int i = 0; i <= COLORMAP_CUSTOM; i++)
      {
      SystemPreset preset = static_cast<SystemPreset>(i);
      m_ColorMapPresetEnumMap.AddPair(preset, this->GetPresetName(preset).c_str());
      }
    }
}

ColorMap
::~ColorMap()
{
}

RegistryEnumMap<ColorMap::SystemPreset> ColorMap::m_ColorMapPresetEnumMap;

bool
ColorMap
::operator==(const ColorMap& rhs) const
{
  // Compare two colormaps for equality
  if (m_CMPoints.size() != rhs.m_CMPoints.size())
    return false;
  CMPointConstIterator it = m_CMPoints.begin();
  CMPointConstIterator rit = rhs.m_CMPoints.begin();
  while (it != m_CMPoints.end())
    {
    if (!(*it == *rit))
      return false;
    ++it;
    ++rit;
    }
  return true;
}

size_t
ColorMap
::InsertInterpolatedCMPoint(double j)
{
  // Create the new point
  RGBAType rgba = MapIndexToRGBA(j);
  CMPoint newbie(j, rgba[0], rgba[1], rgba[2], rgba[3]);

  // Find where to insert the new point
  CMPointIterator it = 
    std::lower_bound(m_CMPoints.begin(), m_CMPoints.end(), newbie);

  // Insert the point after the lower bound
  CMPointIterator itnew = m_CMPoints.insert(it, newbie);

  this->UpdateInterpolants(); 

  m_CMPreset = COLORMAP_CUSTOM;

  return itnew - m_CMPoints.begin();
}


void
ColorMap
::UpdateInterpolants()
{
  // Set the size of the interpolant array to n+1
  size_t n = m_CMPoints.size();
  m_Interpolants.resize(n+1);

  // Loop over the component
  for(unsigned int d = 0; d < 4; d++)
    {
    // Set first and last interpolants
    m_Interpolants[0].slope[d] = 0.0f; 
    m_Interpolants[0].intercept[d] = m_CMPoints[0].m_RGBA[0][d];
    m_Interpolants[n].slope[d] = 0.0f; 
    m_Interpolants[n].intercept[d] = m_CMPoints[n-1].m_RGBA[1][d];

    // Set the intermediate interpolants
    for(size_t i = 1; i < n; i++)
      {
      float t0 = m_CMPoints[i-1].m_Index;
      float t1 = m_CMPoints[i].m_Index;
      float c0 = m_CMPoints[i-1].m_RGBA[1][d];
      float c1 = m_CMPoints[i].m_RGBA[0][d];
      if(t1 > t0)
        {
        m_Interpolants[i].slope[d] = (c1 - c0) / (t1 - t0);
        m_Interpolants[i].intercept[d] = c0 - m_Interpolants[i].slope[d] * t0;
        }
      else
        {
        m_Interpolants[i].slope[d] = 0;
        m_Interpolants[i].intercept[d] = c0;
        }
      }
    }

  // Update state
  this->Modified();
}

ColorMap::RGBAType
ColorMap
::MapIndexToRGBA(double j) const
{
  // We use simple linear search because most colormaps
  // are tiny and the overhead of binary search is not worth it
  size_t n = m_CMPoints.size(), lb;
  for(lb = 0; lb < n; lb++)
    {
    double t = m_CMPoints[lb].m_Index;

    // The right side of this conditional expression is the special case when
    // j equals to the index in the last control point. In that case we want
    // to use the left-side value of the last control point, not the right
    // hand side. Otherwise, the maximum value in the image is treated as
    // being "outside" of the colormap, and for most colormaps is assigned
    // a transparent value. That's undesirable
    if(j < t || (lb == (n-1) && j == t))
      break;
    }

  // Get the interpolants
  const InterpolantData &ic = m_Interpolants[lb];

  // Compute the output value
  RGBAType c;
  c[0] = (unsigned char)(ic.intercept[0] + ic.slope[0] * j);
  c[1] = (unsigned char)(ic.intercept[1] + ic.slope[1] * j);
  c[2] = (unsigned char)(ic.intercept[2] + ic.slope[2] * j);
  c[3] = (unsigned char)(ic.intercept[3] + ic.slope[3] * j);
  return c;
}

void
ColorMap
::PrintSelf(std::ostream & os, itk::Indent indent) const
{
  char buffer[256];
  for(size_t i = 0; i < m_CMPoints.size(); i++)
    {
    CMPoint p = m_CMPoints[i];
    if(p.m_Type == CONTINUOUS)
      {
      snprintf(buffer, sizeof(buffer),
              "%02d-C %7.2f   (%03d %03d %03d %03d)\n", (int) i, p.m_Index, p.m_RGBA[0][0], p.m_RGBA[0][1], p.m_RGBA[0][2], p.m_RGBA[0][3]);
      os << indent << buffer;
      }
    else
      {
      snprintf(buffer, sizeof(buffer),
             "%02d-L %7.2f   (%03d %03d %03d %03d)\n", (int) i, p.m_Index, p.m_RGBA[0][0], p.m_RGBA[0][1], p.m_RGBA[0][2], p.m_RGBA[0][3]);
      os << indent << buffer;
      snprintf(buffer, sizeof(buffer),
              "%02d-R %7.2f   (%03d %03d %03d %03d)\n", (int) i, p.m_Index, p.m_RGBA[1][0], p.m_RGBA[1][1], p.m_RGBA[1][2], p.m_RGBA[1][3]);
      os << indent << buffer;
      }
    }
}
void
ColorMap
::SetToSystemPreset(SystemPreset preset)
{
  m_CMPreset = preset;

  // Handle the special case of COLORMAP_CUSTOM, in which case we don't mess
  // mess with the colormap
  if(preset == COLORMAP_CUSTOM)
    return;

  // The default color for NAN should be something that is distinct from the main
  // color map. We follow the Paraview scheme here and assign it to yellow except
  // for color maps that have lots of yellow in them
  SetNANColor(0xff, 0xff, 0x00, 0xff);

  m_CMPoints.clear();

  const auto addPresetPoints = [this](const PresetColorPoint *points, size_t count)
    {
    for(size_t i = 0; i < count; ++i)
      {
      const PresetColorPoint &p = points[i];
      if(i == 0)
        m_CMPoints.push_back(CMPoint(p.index, p.red, p.green, p.blue, 0x00, 0xff));
      else if(i + 1 == count)
        m_CMPoints.push_back(CMPoint(p.index, p.red, p.green, p.blue, 0xff, 0xff));
      else
        m_CMPoints.push_back(CMPoint(p.index, p.red, p.green, p.blue, 0xff));
      }
    };

  switch(preset)
  {
    case COLORMAP_GREY:
      m_CMPoints.push_back( CMPoint(0.0, 0x00, 0x00, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0, 0xff, 0xff, 0xff, 0xff, 0xff) );
      break;

    case COLORMAP_RED:
      m_CMPoints.push_back( CMPoint(0.0, 0x00, 0x00, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0, 0xff, 0x00, 0x00, 0xff, 0xff) );
      break;

    case COLORMAP_GREEN:
      m_CMPoints.push_back( CMPoint(0.0, 0x00, 0x00, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0, 0x00, 0xff, 0x00, 0xff, 0xff) );
      break;

    case COLORMAP_BLUE:
      m_CMPoints.push_back( CMPoint(0.0, 0x00, 0x00, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0, 0x00, 0x00, 0xff, 0xff, 0xff) );
      break;

    case COLORMAP_HOT:
      SetNANColor(0x00, 0x7f, 0xff, 0xff);
      m_CMPoints.push_back( CMPoint(0.0        , 0x00, 0x00, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint( 2.0 / 63.0, 0x00, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(22.0 / 63.0, 0xd8, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(28.0 / 63.0, 0xff, 0x3a, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(48.0 / 63.0, 0xff, 0xff, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0        , 0xff, 0xff, 0xff, 0xff, 0xff) );
      break;

    case COLORMAP_COOL:
      m_CMPoints.push_back( CMPoint(0.0, 0xff, 0x00, 0xff, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0, 0x00, 0xff, 0xff, 0xff, 0xff) );
      break;

    case COLORMAP_SPRING:
      SetNANColor(0x00, 0x7f, 0xff, 0xff);
      m_CMPoints.push_back( CMPoint(0.0, 0xff, 0x00, 0xff, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0, 0xff, 0xff, 0x00, 0xff, 0xff) );
      break;

    case COLORMAP_SUMMER:
      SetNANColor(0x00, 0x7f, 0xff, 0xff);
      m_CMPoints.push_back( CMPoint(0.0, 0x00, 0x80, 0x66, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0, 0xff, 0xff, 0x66, 0xff, 0xff) );
      break;

    case COLORMAP_AUTUMN:
      SetNANColor(0x00, 0x7f, 0xff, 0xff);
      m_CMPoints.push_back( CMPoint(0.0, 0xff, 0x00, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0, 0xff, 0xff, 0x00, 0xff, 0xff) );
      break;

    case COLORMAP_WINTER:
      m_CMPoints.push_back( CMPoint(0.0, 0x00, 0x00, 0xff, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0, 0x00, 0xff, 0x80, 0xff, 0xff) );
      break;

    case COLORMAP_COPPER:
      SetNANColor(0xff, 0xff, 0xff, 0xff);
      m_CMPoints.push_back( CMPoint(0.0,    0x00, 0x00, 0xff, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(0.8334, 0xff, 0xaa, 0x6a, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0,    0xff, 0xcc, 0x80, 0xff, 0xff) );
      break;

    case COLORMAP_HSV:
      SetNANColor(0xff, 0xff, 0xff, 0xff);
      m_CMPoints.push_back( CMPoint(0.0,    0xff, 0x00, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(0.1667, 0xff, 0xff, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(0.3334, 0x00, 0xff, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(0.5000, 0x00, 0xff, 0xff, 0xff) );
      m_CMPoints.push_back( CMPoint(0.6667, 0x00, 0x00, 0xff, 0xff) );
      m_CMPoints.push_back( CMPoint(0.8334, 0xff, 0x00, 0xff, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0,    0xff, 0x00, 0x00, 0xff, 0xff) );
      break;

    case COLORMAP_JET:
      SetNANColor(0xff, 0xff, 0xff, 0xff);
      m_CMPoints.push_back( CMPoint(0.0,    0x00, 0x00, 0x80, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(0.1   , 0x00, 0x00, 0xff, 0xff) );
      m_CMPoints.push_back( CMPoint(0.36  , 0x00, 0xff, 0xff, 0xff) );
      m_CMPoints.push_back( CMPoint(0.6   , 0xff, 0xff, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(0.9   , 0xff, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0,    0x80, 0x00, 0x00, 0xff, 0xff) );
      break;

    case COLORMAP_BWR:
      m_CMPoints.push_back( CMPoint(0.0,    0x00, 0x00, 0xff, 0x00, 0xff) ); 
      m_CMPoints.push_back( CMPoint(0.5,    0xff, 0xff, 0xff, 0xff) ); 
      m_CMPoints.push_back( CMPoint(1.0,    0xff, 0x00, 0x00, 0xff, 0xff) );
      break;

    case COLORMAP_RWB:
      m_CMPoints.push_back( CMPoint(0.0,    0xff, 0x00, 0x00, 0x00, 0xff) ); 
      m_CMPoints.push_back( CMPoint(0.5,    0xff, 0xff, 0xff, 0xff) ); 
      m_CMPoints.push_back( CMPoint(1.0,    0x00, 0x00, 0xff, 0xff, 0xff) );
      break;

    case COLORMAP_SPEED:
      m_CMPoints.push_back( CMPoint(0.0,    0x00, 0x00, 0xff, 0xff) );
      m_CMPoints.push_back( CMPoint(0.5,    0x00, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0,    0xff, 0xff, 0xff, 0xff) );
      break;

    case COLORMAP_SPEED_OVERLAY:
      m_CMPoints.push_back( CMPoint(0.0,    0xff, 0xff, 0xff, 0x00) );
      m_CMPoints.push_back( CMPoint(0.4,    0xff, 0xff, 0xff, 0x00) );
      m_CMPoints.push_back( CMPoint(0.6,    0xff, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(1.0,    0xff, 0x00, 0x00, 0xff) );
      break;

    case COLORMAP_LEVELSET:
      m_CMPoints.push_back( CMPoint(0.0,    0xff, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(0.4,    0xff, 0x00, 0x00, 0xff) );
      m_CMPoints.push_back( CMPoint(0.6,    0xff, 0xff, 0xff, 0x00) );
      m_CMPoints.push_back( CMPoint(1.0,    0xff, 0xff, 0xff, 0x00) );
      break;

    case COLORMAP_INFERNO:
      SetNANColor(0x00, 0xff, 0xff, 0xff);
      addPresetPoints(INFERNO_POINTS, sizeof(INFERNO_POINTS) / sizeof(INFERNO_POINTS[0]));
      break;

    case COLORMAP_VIRIDIS:
      SetNANColor(0xff, 0x00, 0xff, 0xff);
      addPresetPoints(VIRIDIS_POINTS, sizeof(VIRIDIS_POINTS) / sizeof(VIRIDIS_POINTS[0]));
      break;

    case COLORMAP_PLASMA:
      SetNANColor(0x00, 0xff, 0xff, 0xff);
      addPresetPoints(PLASMA_POINTS, sizeof(PLASMA_POINTS) / sizeof(PLASMA_POINTS[0]));
      break;

    case COLORMAP_MAGMA:
      SetNANColor(0x00, 0xff, 0xff, 0xff);
      addPresetPoints(MAGMA_POINTS, sizeof(MAGMA_POINTS) / sizeof(MAGMA_POINTS[0]));
      break;

    case COLORMAP_CIVIDIS:
      SetNANColor(0xff, 0x00, 0xff, 0xff);
      addPresetPoints(CIVIDIS_POINTS, sizeof(CIVIDIS_POINTS) / sizeof(CIVIDIS_POINTS[0]));
      break;

    case COLORMAP_TURBO:
      SetNANColor(0xff, 0xff, 0xff, 0xff);
      addPresetPoints(TURBO_POINTS, sizeof(TURBO_POINTS) / sizeof(TURBO_POINTS[0]));
      break;

    case COLORMAP_CUSTOM:
      break;
   }

  this->UpdateInterpolants();
}

void ColorMap::UpdateCMPoint(size_t j, const ColorMap::CMPoint &p)
{
  if(m_CMPoints[j] != p)
    {
    m_CMPoints[j] = p;
    this->UpdateInterpolants();
    m_CMPreset = COLORMAP_CUSTOM;
    }
}

void ColorMap::DeleteCMPoint(size_t j)
{
  m_CMPoints.erase(m_CMPoints.begin() + j);
  this->UpdateInterpolants();
  m_CMPreset = COLORMAP_CUSTOM;
}


void ColorMap
::SaveToRegistry(Registry &reg)
{
  // Save what system preset we are using (if any)
  reg.Entry("Preset").PutEnum(m_ColorMapPresetEnumMap, m_CMPreset);

  // For custom colormaps, save them
  if(m_CMPreset == COLORMAP_CUSTOM)
    {
    // Store the number of control points
    reg["NumberOfControlPoints"] << m_CMPoints.size();

    // Store the NaN color
    reg["NaNColor.R"] << (int) m_NANColor[0];
    reg["NaNColor.G"] << (int) m_NANColor[1];
    reg["NaNColor.B"] << (int) m_NANColor[2];
    reg["NaNColor.A"] << (int) m_NANColor[3];

    RegistryEnumMap<CMPointType> emap;
    emap.AddPair(CONTINUOUS,"Continuous");
    emap.AddPair(DISCONTINUOUS,"Discontinuous");

    // Save each control point
    for(size_t iPoint = 0; iPoint < m_CMPoints.size(); iPoint++)
      {
      // Get the current values, just in case
      CMPoint p = m_CMPoints[iPoint];

      // Create a folder in the registry
      std::string key = reg.Key("ControlPoint[%04d]",iPoint);
      Registry &folder = reg.Folder(key);
      folder["Index"] << p.m_Index;
      folder["Type"].PutEnum(emap, p.m_Type);
      folder["Left.R"] << (int) p.m_RGBA[0][0];
      folder["Left.G"] << (int) p.m_RGBA[0][1];
      folder["Left.B"] << (int) p.m_RGBA[0][2];
      folder["Left.A"] << (int) p.m_RGBA[0][3];
      folder["Right.R"] << (int) p.m_RGBA[1][0];
      folder["Right.G"] << (int) p.m_RGBA[1][1];
      folder["Right.B"] << (int) p.m_RGBA[1][2];
      folder["Right.A"] << (int) p.m_RGBA[1][3];
      }
    }
}

// TODO: the colormap should know if it is in a given system preset or not, and if
// so, only save the preset name.
void ColorMap
::LoadFromRegistry(Registry &reg)
{
  // See what system preset we are using (if any)
  SystemPreset preset =
      reg.Entry("Preset").GetEnum(m_ColorMapPresetEnumMap, COLORMAP_GREY);

  // If system preset, apply it and we're done
  if(preset != COLORMAP_CUSTOM)
    {
    SetToSystemPreset(preset);
    }
  else
    {
    // Store the number of control points
    size_t n = reg["NumberOfControlPoints"][0];
    if(n == 0)
      return;

    // Read each control point
    CMPointList newpts;

    RegistryEnumMap<CMPointType> emap;
    emap.AddPair(CONTINUOUS,"Continuous");
    emap.AddPair(DISCONTINUOUS,"Discontinuous");

    // Read the NaN color
    m_NANColor[0] = (unsigned char) reg["NaNColor.R"][0xff];
    m_NANColor[1] = (unsigned char) reg["NaNColor.G"][0xff];
    m_NANColor[2] = (unsigned char) reg["NaNColor.B"][0x00];
    m_NANColor[3] = (unsigned char) reg["NaNColor.A"][0xff];

    // Save each control point
    for(size_t iPoint = 0; iPoint < n; iPoint++)
      {
      // Get the current values, just in case
      CMPoint p;

      // Create a folder in the registry
      std::string key = reg.Key("ControlPoint[%04d]",iPoint);
      Registry &folder = reg.Folder(key);

      p.m_Index = folder["Index"][-1.0];
      p.m_Type = folder["Type"].GetEnum(emap, CONTINUOUS);
      p.m_RGBA[0][0] = (unsigned char) folder["Left.R"][0];
      p.m_RGBA[0][1] = (unsigned char) folder["Left.G"][0];
      p.m_RGBA[0][2] = (unsigned char) folder["Left.B"][0];
      p.m_RGBA[0][3] = (unsigned char) folder["Left.A"][0];
      if(p.m_Type == CONTINUOUS)
        {
        p.m_RGBA[1][0] = p.m_RGBA[0][0];
        p.m_RGBA[1][1] = p.m_RGBA[0][1];
        p.m_RGBA[1][2] = p.m_RGBA[0][2];
        p.m_RGBA[1][3] = p.m_RGBA[0][3];
        }
      else
        {
        p.m_RGBA[1][0] = (unsigned char) folder["Right.R"][0];
        p.m_RGBA[1][1] = (unsigned char) folder["Right.G"][0];
        p.m_RGBA[1][2] = (unsigned char) folder["Right.B"][0];
        p.m_RGBA[1][3] = (unsigned char) folder["Right.A"][0];
        }

      // Check validity
      if(iPoint == 0 && p.m_Index != 0.0)
        throw IRISException("Can not read color map. First point has non-zero index.");

      if(iPoint == n-1 && p.m_Index != 1.0)
        throw IRISException("Can not read color map. Last point has index not equal to 1.");

      if(iPoint > 0 && p.m_Index < newpts[iPoint-1].m_Index)
        throw IRISException("Can not read color map. Indices are not stored in order.");

      newpts.push_back(p);
      }

    // Got this far? store the new map
    m_CMPoints = newpts;
    this->UpdateInterpolants();

    m_CMPreset = COLORMAP_CUSTOM;
    }
  }

void ColorMap::SetNANColor(EltType r, EltType g, EltType b, EltType alpha)
{
  RGBAType pixel;
  pixel[0] = r;
  pixel[1] = g;
  pixel[2] = b;
  pixel[3] = alpha;
  SetNANColor(pixel);
}

void ColorMap::CopyInformation(const itk::DataObject *source)
{
  const ColorMap *cm_source = dynamic_cast<const ColorMap *>(source);
  m_CMPreset = cm_source->m_CMPreset;
  m_CMPoints = cm_source->m_CMPoints;
  m_Interpolants = cm_source->m_Interpolants;
  m_NANColor = cm_source->m_NANColor;
  this->Modified();
}

AbstractColorMapPresetNameSource *ColorMap::m_ColorMapPresetNameSource = nullptr;

void
ColorMap::SetColorMapPresetNameSource(AbstractColorMapPresetNameSource *source)
{
  m_ColorMapPresetNameSource = source;
}



std::string
ColorMap::GetPresetName(ColorMap::SystemPreset preset)
{
  if(!m_ColorMapPresetNameSource)
    throw IRISException("m_ColorMapPresetNameSource not set in ColorMap::GetPresetName");

  return m_ColorMapPresetNameSource->GetPresetName(preset, false);
}
