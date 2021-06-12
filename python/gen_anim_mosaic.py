#
#  gen_anim_mosaic.py
#
#  Copyright (C) 2021 Zhang Maiyun <myzhang1029@hotmail.com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

"""Generate animated photo mosaic tiles."""

import random
from typing import Dict, Tuple, Any, List, Optional
from PIL import Image, UnidentifiedImageError
from math import gcd
import cv2


class Mosaic:
    """Animated photo Mosaic generator.

    self.n_width and n_height are suggested values for number of images
        on each width/height.
    self.sources are paths to tile images/videos.
    self.baseimg is the path to the background/main image.
    self.opacity is the opacity of the tiles (on the BASEIMG).
    """

    def __init__(self, sources: List[str], baseimg: str,
                 n_width: int = 0, n_height: int = 0):
        self.n_width = n_width
        self.n_height = n_height
        self.sources = sources
        self.baseimg = Image.open(baseimg)
        # Let the resulting image be of the same size as the base
        self.img_w = self.baseimg.size[0]
        self.img_h = self.baseimg.size[1]
        # Default number of images on each side
        n_width = n_width or len(sources)
        n_height = n_height or len(sources)
        # And find the number of images per side
        self.n_width = self.find_ceil_factor(n_width, self.img_w)
        self.n_height = self.find_ceil_factor(n_height, self.img_h)
        # Hence get the size of a single tile
        self.tile_w = self.img_w // self.n_width
        self.tile_h = self.img_h // self.n_height
        # Generate locations for images
        self.gen_pairs()
        # Cached results
        self.img_cache: Dict[str, Image.Image] = {}
        # Global opacity
        self.opacity = 0.7

    def gen_pairs(self):
        """Generate all coordinates and map them to a random image."""
        coords = ((w, h) for w in range(self.n_width)
                  for h in range(self.n_height))
        self.pairs = {coord: random.choice(self.sources) for coord in coords}

    def get_pairs(self) -> Dict[Tuple[int, int], str]:
        return self.pairs

    def get_link_back(self) -> Dict[str, List[Tuple[int, int]]]:
        """Get a dict of objects to coords from the result of gen_pairs."""
        result: Dict[str, List[Tuple[int, int]]] = {}
        for coord in self.pairs:
            result[self.pairs[coord]] = []
        for coord in self.pairs:
            result[self.pairs[coord]].append(coord)
        return result

    @staticmethod
    def find_ceil_factor(smaller: int, larger: int):
        """Find the next factor of LARGER >= SMALLER."""
        if smaller > larger:
            # EINVAL
            return smaller
        while larger % smaller:
            smaller += 1
        return smaller

    def process_image(self, name: str) -> Image.Image:
        """Generate tiles."""
        try:
            img = Image.open(name)
        except UnidentifiedImageError:
            # Might be a video
            capt = cv2.VideoCapture(name)
            total_frames = capt.get(cv2.CAP_PROP_FRAME_COUNT)
            # The first frame might be black
            success, imga = capt.read()
            if total_frames > 1 or not success:
                success, imga = capt.read()
                if not success:
                    raise UnidentifiedImageError("Cannot use this image")
            img = Image.fromarray(imga)
        return img.resize((self.tile_w, self.tile_h))

    def set_opacity(self, opacity: float):
        """Set global opacity and reset cache."""
        self.opacity = opacity
        self.img_cache = {}

    def make_mosaic(self, only: Optional[List[str]] = None) -> Image.Image:
        """Make mosaic image.

        If ONLY is set, only images in ONLY will be rendered, other
        ones are alpha.
        """
        # Result image
        result = self.baseimg.convert("RGBA").copy()
        # Alpha image
        alpha = Image.new("RGBA", (self.tile_w, self.tile_h),
                          color=(0, 0, 0, 0))
        for coord in self.pairs:
            img_name = self.pairs[coord]
            paste_loc = (coord[0] * self.tile_w, coord[1] * self.tile_h)
            if not only or img_name in only:
                # Cache processed images
                if img_name in self.img_cache:
                    image = self.img_cache[img_name]
                else:
                    image = self.process_image(img_name)
                    self.img_cache[img_name] = image
                image.putalpha(int(self.opacity * 255))
                result.paste(image, paste_loc, image)
            else:
                # Make sure no mask is used when pasting alpha
                result.paste(alpha, paste_loc)
        return result

    def make_serie(self) -> List[Image.Image]:
        """Progressively make a list of images showing "unveiling tiles"."""
        result: List[Image.Image] = []
        for n in range(len(self.sources)):
            only = self.sources[0:n+1]
            result.append(self.make_mosaic(only))
        return result
