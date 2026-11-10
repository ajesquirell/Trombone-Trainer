"""Render the same PGE3 geometry offline. Requires Pillow only for this tool.

Compile tools/trombone_preview.cpp, then pass its executable as the first argument.
Output: build-art-preview/trombone-{1,4,7,compact}.png and .svg.
"""
import json
from pathlib import Path
import subprocess
import sys
from PIL import Image, ImageDraw

executable = str(Path(sys.argv[1]).resolve())
output = Path('build-art-preview')
output.mkdir(exist_ok=True)
for name, position, width, height in [('1', 1, 1100, 340), ('4', 4, 1100, 340),
                                       ('7', 7, 1100, 340), ('compact', 7, 390, 145)]:
    path = output / ('trombone-' + name)
    subprocess.run([executable, str(path) + '.json', str(position), str(width), str(height)], check=True)
    shapes = json.loads(Path(str(path) + '.json').read_text())
    image = Image.new('RGB', (width * 3, height * 3), (11, 22, 38))
    canvas = ImageDraw.Draw(image)
    svg = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}"><rect width="100%" height="100%" fill="#0b1626"/>']
    for shape in shapes:
        points, color = shape['p'], tuple(shape['color'])
        if shape['kind'] == 'triangle':
            vertices = [(points[i], points[i + 1]) for i in range(0, 6, 2)]
            assert all(0 <= x <= width and 0 <= y <= height for x, y in vertices)
            canvas.polygon([(x * 3, y * 3) for x, y in vertices], fill=color)
            coords = ' '.join(f'{x},{y}' for x, y in vertices)
            svg.append(f'<polygon points="{coords}" fill="rgb{color}"/>')
        else:
            x, y, radius = points
            assert 0 <= x - radius <= x + radius <= width
            assert 0 <= y - radius <= y + radius <= height
            canvas.ellipse(((x - radius) * 3, (y - radius) * 3,
                            (x + radius) * 3, (y + radius) * 3), fill=color)
            svg.append(f'<circle cx="{x}" cy="{y}" r="{radius}" fill="rgb{color}"/>')
    image.resize((width, height), getattr(Image, 'Resampling', Image).LANCZOS).save(str(path) + '.png')
    Path(str(path) + '.svg').write_text(''.join(svg) + '</svg>')
    print(path.with_suffix('.png'))
