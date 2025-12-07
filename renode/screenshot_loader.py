"""
Screenshot Loader for Virtual LCD Monitor
Bu script screenshot-u virtual LCD-də göstərir
"""

from PIL import Image, ImageTk
import tkinter as tk
from lcd_monitor_gui import VirtualLCDMonitor

class ScreenshotLoader:
    """Screenshot-u virtual LCD-də göstərir"""
    
    def __init__(self, screenshot_path, lcd_monitor):
        self.screenshot_path = screenshot_path
        self.lcd = lcd_monitor
        
    def load_screenshot(self):
        """Screenshot-u yüklə və LCD-də göstər"""
        try:
            # Screenshot-u yüklə
            img = Image.open(self.screenshot_path)
            
            # 320x240 ölçüsünə resize et
            img = img.resize((320, 240), Image.Resampling.LANCZOS)
            
            # RGB-yə çevir (RGB565 üçün)
            if img.mode != 'RGB':
                img = img.convert('RGB')
            
            # Pixel-ləri LCD-yə köçür
            pixels = img.load()
            for y in range(240):
                for x in range(320):
                    r, g, b = pixels[x, y]
                    # RGB888-dən RGB565-ə çevir
                    color = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
                    self.lcd.update_pixel(x, y, color)
            
            # Display-i yenilə
            self.lcd.update_display()
            
            print(f"Screenshot loaded: {self.screenshot_path}")
            print("Display updated!")
            
        except Exception as e:
            print(f"Error loading screenshot: {e}")

def main():
    """Main function"""
    import sys
    
    if len(sys.argv) < 2:
        print("Usage: python screenshot_loader.py <screenshot_path>")
        print("Example: python screenshot_loader.py screenshot.jpg")
        return
    
    screenshot_path = sys.argv[1]
    
    print("Loading screenshot into Virtual LCD Monitor...")
    print(f"Screenshot: {screenshot_path}")
    print("")
    
    # Create LCD monitor
    lcd = VirtualLCDMonitor(320, 240, scale=2)
    
    # Load screenshot
    loader = ScreenshotLoader(screenshot_path, lcd)
    loader.load_screenshot()
    
    print("")
    print("Virtual LCD Monitor is showing the screenshot")
    print("Close the window to exit")
    
    # Run LCD monitor
    lcd.run()

if __name__ == "__main__":
    main()

