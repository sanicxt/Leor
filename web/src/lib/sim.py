def draw_rect(grid, x0, y0, x1, y1, color):
    l, r = min(x0, x1), max(x0, x1)
    t, b = min(y0, y1), max(y0, y1)
    for y in range(t, b):
        for x in range(l, r):
            grid[y][x] = color

def draw_triangle(grid, x0, y0, x1, y1, x2, y2, color):
    # Dumb rasterizer
    min_x, max_x = min(x0, x1, x2), max(x0, x1, x2)
    min_y, max_y = min(y0, y1, y2), max(y0, y1, y2)
    def sign(p1x, p1y, p2x, p2y, p3x, p3y):
        return (p1x - p3x) * (p2y - p3y) - (p2x - p3x) * (p1y - p3y)
    for y in range(min_y, max_y + 1):
        for x in range(min_x, max_x + 1):
            d1 = sign(x, y, x0, y0, x1, y1)
            d2 = sign(x, y, x1, y1, x2, y2)
            d3 = sign(x, y, x2, y2, x0, y0)
            has_neg = (d1 < 0) or (d2 < 0) or (d3 < 0)
            has_pos = (d1 > 0) or (d2 > 0) or (d3 > 0)
            if not (has_neg and has_pos):
                grid[y][x] = color

# AWE config
H, W = 45, 35
rBot = 12
slope = 0.1
# Let's say scale is 1.0
cx = 50
cy = 50
delta_y_bottom = int(H * slope / 2.0)
BLcx = cx - W//2 + rBot
BRcx = cx + W//2 - rBot
BLcy = cy + H//2 - rBot - delta_y_bottom
BRcy = cy + H//2 - rBot + delta_y_bottom

grid = [['.' for _ in range(100)] for _ in range(100)]
MAIN = '#'
BG = '.'

draw_rect(grid, BLcx, BLcy, BRcx, BRcy + rBot, MAIN)
print("Before triangles:")
for y in range(BLcy, BRcy + rBot + 2): print(f"{y:2}|" + "".join(grid[y][BLcx-rBot:BRcx+rBot+2]))

# New logic
X0, Y0 = BRcx + rBot - 1, BRcy + rBot - 1
X1, Y1 = BLcx - rBot, BLcy + rBot - 1
X2, Y2 = BLcx - rBot, BRcy + rBot - 1
draw_triangle(grid, X0, Y0, X1, Y1, X2, Y2, BG)
draw_triangle(grid, X1, Y1, X0, Y0, X0, Y1, MAIN)

print("After triangles:")
for y in range(BLcy, BRcy + rBot + 2): print(f"{y:2}|" + "".join(grid[y][BLcx-rBot:BRcx+rBot+2]))

