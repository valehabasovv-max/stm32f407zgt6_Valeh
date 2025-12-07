"""
Virtual LCD Monitor GUI for Renode
This creates a visual display window showing the LCD content from STM32
Monitors FSMC writes to LCD area (0x60000000) and displays them
"""

import tkinter as tk
from tkinter import Canvas
import threading
import time
import struct

class VirtualLCDMonitor:
    """Virtual LCD Monitor - 320x240 RGB565 Display"""
    
    def __init__(self, width=320, height=240, scale=2):
        self.width = width
        self.height = height
        self.scale = scale
        self.framebuffer = [[0x0000 for x in range(width)] for y in range(height)]
        
        # Create GUI
        self.root = tk.Tk()
        self.root.title("VALEH Pressure Control - Virtual LCD Monitor")
        self.root.geometry(f"{width * scale + 20}x{height * scale + 60}")
        self.root.configure(bg="black")
        
        # Canvas for display
        self.canvas = Canvas(self.root, 
                            width=width * scale, 
                            height=height * scale,
                            bg="black",
                            highlightthickness=0)
        self.canvas.pack(padx=10, pady=10)
        
        # Status label
        self.status_label = tk.Label(self.root, 
                                     text="Virtual LCD Monitor - Waiting for data...",
                                     bg="black", 
                                     fg="white",
                                     font=("Arial", 10))
        self.status_label.pack()
        
        # Update thread
        self.running = True
        self.update_thread = threading.Thread(target=self.update_loop, daemon=True)
        self.update_thread.start()
        
        # Close handler
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)
    
    def rgb565_to_rgb(self, color):
        """Convert RGB565 to RGB888"""
        r = ((color >> 11) & 0x1F) << 3
        g = ((color >> 5) & 0x3F) << 2
        b = (color & 0x1F) << 3
        return (r, g, b)
    
    def update_pixel(self, x, y, color):
        """Update a single pixel"""
        if 0 <= x < self.width and 0 <= y < self.height:
            self.framebuffer[y][x] = color & 0xFFFF
    
    def update_display(self):
        """Update the entire display"""
        self.canvas.delete("all")
        for y in range(self.height):
            for x in range(self.width):
                color = self.framebuffer[y][x]
                r, g, b = self.rgb565_to_rgb(color)
                hex_color = f"#{r:02x}{g:02x}{b:02x}"
                
                # Draw rectangle (scaled)
                self.canvas.create_rectangle(
                    x * self.scale, y * self.scale,
                    (x + 1) * self.scale, (y + 1) * self.scale,
                    fill=hex_color,
                    outline=""
                )
        
        self.status_label.config(text=f"Virtual LCD Monitor - {self.width}x{self.height} (Updated)")
        self.root.update()
    
    def update_loop(self):
        """Background update loop"""
        while self.running:
            try:
                self.update_display()
                time.sleep(0.1)  # Update every 100ms
            except:
                pass
    
    def on_closing(self):
        """Handle window closing"""
        self.running = False
        self.root.destroy()
    
    def run(self):
        """Run the GUI loop"""
        self.root.mainloop()
    
    def simulate_test_pattern(self):
        """Simulate a test pattern for demonstration"""
        for y in range(self.height):
            for x in range(self.width):
                # Create a test pattern
                r = (x * 32) // self.width
                g = (y * 64) // self.height
                b = ((x + y) * 32) // (self.width + self.height)
                color = (r << 11) | (g << 5) | b
                self.update_pixel(x, y, color)
        self.update_display()

# Usage
if __name__ == "__main__":
    print("Virtual LCD Monitor Starting...")
    print("This will show a virtual LCD display")
    print("Note: To connect with Renode, you need to implement FSMC write hooking")
    print("")
    
    lcd = VirtualLCDMonitor(320, 240, scale=2)
    
    # Show test pattern
    print("Showing test pattern...")
    lcd.simulate_test_pattern()
    
    print("Virtual LCD Monitor is running...")
    print("Close the window to exit")
    
    lcd.run()

