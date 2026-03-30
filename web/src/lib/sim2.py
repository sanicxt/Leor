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
    
    # B_R
    if corner == "B_R":
        x = 0; y = ry; s = 2*ry2 + rx2*(1 - 2*ry)
        while ry2*x <= rx2*y:
            for X in range(x0, x0+x): grid[y0+y-1][X] = color
            if s >= 0:
                s += fx2*(1-y); y -= 1
            s += ry2*(4*x+6); x += 1
            
        x = rx; y = 0; s = 2*rx2 + ry2*(1 - 2*rx)
        while rx2*y <= ry2*x:
            for X in range(x0, x0+x): grid[y0+y-1][X] = color
            if s >= 0:
                s += fy2*(1-x); x -= 1
            s += rx2*(4*y+6); y += 1

    # B_L
    if corner == "B_L":
        x = 0; y = ry; s = 2*ry2 + rx2*(1 - 2*ry)
        while ry2*x <= rx2*y:
            for X in range(x0-x, x0): grid[y0+y-1][X] = color
            if s >= 0:
                s += fx2*(1-y); y -= 1
            s += ry2*(4*x+6); x += 1
            
        x = rx; y = 0; s = 2*rx2 + ry2*(1 - 2*rx)
        while rx2*y <= ry2*x:
            for X in range(x0-x, x0): grid[y0+y-1][X] = color
            if s >= 0:
                s += fy2*(1-x); x -= 1
            s += rx2*(4*y+6); y += 1

H, W = 45, 35
rBot = 12
slope = 0.1
cx, cy = 50, 50
delta_y_bottom = int(H * slope / 2.0)
BLcx = cx - W//2 + rBot
BRcx = cx + W//2 - rBot
BLcy = cy + H//2 - rBot - delta_y_bottom
BRcy = cy + H//2 - rBot + delta_y_bottom

grid = [['.' for _ in range(100)] for _ in range(100)]
draw_rect(grid, BLcx, BLcy, BRcx, BRcy + rBot, '#')
draw_ellipse_corner(grid, "B_L", BLcx, BLcy, rBot, rBot, '#')
draw_ellipse_corner(grid, "B_R", BRcx, BRcy, rBot, rBot, '#')

for y in range(BLcy, BRcy + rBot + 2): print("B: " + "".join(grid[y][BLcx-rBot:BRcx+rBot+2]))

