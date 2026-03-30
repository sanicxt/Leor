def draw_rect(grid, x0, y0, x1, y1, color):
    l, r = min(x0, x1), max(x0, x1)
    t, b = min(y0, y1), max(y0, y1)
    for y in range(t, b):
        for x in range(l, r):
            grid[y][x] = color

def draw_ellipse_corner(grid, corner, x0, y0, rx, ry, color):
    rx2 = rx*rx
    ry2 = ry*ry
    fx2 = 4*rx2
    fy2 = 4*ry2
    if corner == 'B_R':
        x = 0; y = ry; s = 2*ry2 + rx2*(1 - 2*ry)
        while ry2*x <= rx2*y:
            for X in range(x0, x0+x): grid[y0+y-1][X] = color
            if s >= 0: s += fx2*(1-y); y -= 1
            s += ry2*(4*x+6); x += 1
        x = rx; y = 0; s = 2*rx2 + ry2*(1 - 2*rx)
        while rx2*y <= ry2*x:
            for X in range(x0, x0+x): grid[y0+y-1][X] = color
            if s >= 0: s += fy2*(1-x); x -= 1
            s += rx2*(4*y+6); y += 1

def draw_triangle(grid, x0, y0, x1, y1, x2, y2, color):
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

H, W = 45, 35
rBot = 12
slope = 0.1
cx, cy = 50, 50
delta_y_bottom = int(H * slope / 2.0)
BLcx = cx - W//2 + rBot
BRcx = cx + W//2 - rBot
BLcy = cy + H//2 - rBot - delta_y_bottom
BRcy = cy + H//2 - rBot + delta_y_bottom

print("Current Bug (Notch):")
grid1 = [['.' for _ in range(100)] for _ in range(100)]
draw_rect(grid1, BLcx, BLcy, BRcx, BRcy + rBot, '#')
draw_triangle(grid1, BRcx + rBot - 1, BRcy + rBot - 1, BLcx - rBot, BLcy + rBot - 1, BLcx - rBot, BRcy + rBot - 1, '.')
draw_ellipse_corner(grid1, 'B_R', BRcx, BRcy, rBot, rBot, '#')
for y in range(BLcy, BRcy + rBot + 2): print(''.join(grid1[y][BLcx-rBot:BRcx+rBot+2]))

print("\nMasking AFTER Corners:")
grid2 = [['.' for _ in range(100)] for _ in range(100)]
draw_rect(grid2, BLcx, BLcy, BRcx, BRcy + rBot, '#')
draw_ellipse_corner(grid2, 'B_R', BRcx, BRcy, rBot, rBot, '#')
draw_triangle(grid2, BRcx + rBot - 1, BRcy + rBot - 1, BLcx - rBot, BLcy + rBot - 1, BLcx - rBot, BRcy + rBot - 1, '.')
for y in range(BLcy, BRcy + rBot + 2): print(''.join(grid2[y][BLcx-rBot:BRcx+rBot+2]))

