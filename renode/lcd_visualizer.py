"""
Virtual LCD Visualizer for Renode
This script creates a visual display window showing the LCD content
"""

import sys
import struct
try:
    import tkinter as tk
    from tkinter import Canvas
    HAS_TKINTER = True
except ImportError:
    HAS_TKINTER = False
    print("Warning: tkinter not available. Install it for GUI display.")

class VirtualLCD:
    """Virtual LCD Display for ILI9341 (320x240)"""
    
    def __init__(self, width=320, height=240, scale=2):
        self.width = width
        self.height = height
        self.scale = scale
        self.framebuffer = [[0x0000 for x in range(width)] for y in range(height)]
        
        if HAS_TKINTER:
            self.root = tk.Tk()
            self.root.title("VALEH Pressure Control - Virtual LCD")
            self.root.geometry(f"{width * scale}x{height * scale}")
            
            self.canvas = Canvas(self.root, 
                                width=width * scale, 
                                height=height * scale,
                                bg="black")
            self.canvas.pack()
            
            # Update display
            self.update_display()
        else:
            print("LCD Visualizer initialized (no GUI)")
    
    def update_pixel(self, x, y, color):
        """Update a single pixel"""
        if 0 <= x < self.width and 0 <= y < self.height:
            self.framebuffer[y][x] = color
            if HAS_TKINTER:
                self.draw_pixel(x, y, color)
    
    def draw_pixel(self, x, y, color):
        """Draw a pixel on canvas"""
        # Convert RGB565 to RGB
        r = ((color >> 11) & 0x1F) << 3
        g = ((color >> 5) & 0x3F) << 2
        b = (color & 0x1F) << 3
        
        # Draw rectangle (scaled)
        self.canvas.create_rectangle(
            x * self.scale, y * self.scale,
            (x + 1) * self.scale, (y + 1) * self.scale,
            fill=f"#{r:02x}{g:02x}{b:02x}",
            outline=""
        )
    
    def update_display(self):
        """Update the entire display"""
        if HAS_TKINTER:
            self.canvas.delete("all")
            for y in range(self.height):
                for x in range(self.width):
                    self.draw_pixel(x, y, self.framebuffer[y][x])
            self.root.update()
    
    def run(self):
        """Run the GUI loop"""
        if HAS_TKINTER:
            self.root.mainloop()

# Usage example
if __name__ == "__main__":
    lcd = VirtualLCD(320, 240, scale=2)
    
    # Test: Draw some colors
    for y in range(240):
        for x in range(320):
            # Create a test pattern
            color = ((x * 32) // 320) << 11 | ((y * 64) // 240) << 5 | ((x + y) * 32) // 560
            lcd.update_pixel(x, y, color)
    
    lcd.update_display()
    lcd.run()

