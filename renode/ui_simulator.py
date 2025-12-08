"""
VALEH Pressure Control UI Simulator
Proyektin UI kodlarını analiz edib virtual LCD-də eyni UI-ni göstərir
C kodlarından konfiqurasiya avtomatik oxunur
"""

import tkinter as tk
from tkinter import Canvas, font
import math
import os
from parse_c_config import parse_ui_config

# Default rənglər (C kodlarından oxunacaq, yoxdursa bunlar istifadə olunur)
COLOR_BG_DARK = 0x0821
COLOR_BG_PANEL = 0x1082
COLOR_ACCENT_BLUE = 0x04DF
COLOR_ACCENT_GREEN = 0x07E0
COLOR_ACCENT_RED = 0xF800
COLOR_ACCENT_YELLOW = 0xFFE0
COLOR_TEXT_WHITE = 0xFFFF
COLOR_TEXT_GREY = 0x8410
COLOR_BORDER = 0x4208
COLOR_CYAN = 0x07FF
COLOR_ACCENT_ORANGE = 0xFD20

def rgb565_to_rgb(color):
    """Convert RGB565 to RGB888"""
    r = ((color >> 11) & 0x1F) << 3
    g = ((color >> 5) & 0x3F) << 2
    b = (color & 0x1F) << 3
    return (r, g, b)

def rgb_to_hex(r, g, b):
    """Convert RGB to hex string"""
    return f"#{r:02x}{g:02x}{b:02x}"

class VALEH_UISimulator:
    """VALEH Pressure Control UI Simulator"""
    
    def __init__(self, width=320, height=240, scale=2):
        # C kodlarından konfiqurasiya oxu
        project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        self.ui_config, self.c_defines = parse_ui_config(project_root)
        
        # Ölçülər (C kodundan və ya default)
        self.width = self.ui_config.get('dimensions', {}).get('width', width)
        self.height = self.ui_config.get('dimensions', {}).get('height', height)
        self.scale = scale
        
        # Rənglər (C kodundan və ya default) - instance variables kimi saxla
        colors = self.ui_config.get('colors', {})
        self.COLOR_BG_DARK = colors.get('COLOR_BG_DARK', COLOR_BG_DARK)
        self.COLOR_BG_PANEL = colors.get('COLOR_BG_PANEL', COLOR_BG_PANEL)
        self.COLOR_ACCENT_BLUE = colors.get('COLOR_ACCENT_BLUE', COLOR_ACCENT_BLUE)
        self.COLOR_ACCENT_GREEN = colors.get('COLOR_ACCENT_GREEN', COLOR_ACCENT_GREEN)
        self.COLOR_ACCENT_RED = colors.get('COLOR_ACCENT_RED', COLOR_ACCENT_RED)
        self.COLOR_ACCENT_YELLOW = colors.get('COLOR_ACCENT_YELLOW', COLOR_ACCENT_YELLOW)
        self.COLOR_TEXT_WHITE = colors.get('COLOR_TEXT_WHITE', COLOR_TEXT_WHITE)
        self.COLOR_TEXT_GREY = colors.get('COLOR_TEXT_GREY', COLOR_TEXT_GREY)
        self.COLOR_BORDER = colors.get('COLOR_BORDER', COLOR_BORDER)
        self.COLOR_ACCENT_ORANGE = colors.get('COLOR_ACCENT_ORANGE', COLOR_ACCENT_ORANGE)
        self.COLOR_CYAN = 0x07FF  # ILI9341_COLOR_CYAN
        
        # UI state
        self.current_pressure = 100.5
        self.target_pressure = 100.0
        self.motor_pwm = 45.2
        self.zme_pwm = 65.8
        self.drv_pwm = 32.1
        self.error = 0.5
        self.system_running = True
        self.control_enabled = True
        self.current_preset = 1
        self.coord_mode = 0
        self.current_page = "main"  # "main", "menu", "setpoint", "pid_tune", "calibration"
        
        # PID parametrləri
        self.pid_kp = 1.5
        self.pid_ki = 0.05
        self.pid_kd = 0.1
        
        # Calibration dəyərləri
        self.raw_adc_value = 2048
        
        # Preset dəyərləri (C kodundan və ya default)
        self.presets = self.ui_config.get('presets', [50.0, 100.0, 150.0, 200.0, 250.0, 300.0])
        self.preset_names = self.ui_config.get('preset_names', ["LOW", "MED", "HIGH", "V.HI", "EXT", "MAX"])
        
        # Preset rəngləri (default, C kodundan parse edilə bilər)
        self.preset_colors = [
            self.COLOR_ACCENT_GREEN, self.COLOR_CYAN, self.COLOR_ACCENT_YELLOW,
            self.COLOR_ACCENT_ORANGE,
            self.COLOR_ACCENT_RED, 0xF81F  # Magenta
        ]
        
        print(f"✅ UI Configuration loaded from C code:")
        print(f"   Screen: {self.width}x{self.height}")
        print(f"   Presets: {self.presets}")
        print(f"   Preset Names: {self.preset_names}")
        
        # Create GUI
        self.root = tk.Tk()
        self.root.title("VALEH Pressure Control - UI Simulator")
        self.root.geometry(f"{width * scale + 20}x{height * scale + 40}")
        self.root.configure(bg="black")
        
        # Canvas
        self.canvas = Canvas(self.root,
                            width=width * scale,
                            height=height * scale,
                            bg=rgb_to_hex(*rgb565_to_rgb(COLOR_BG_DARK)),
                            highlightthickness=0)
        self.canvas.pack(padx=10, pady=10)
        
        # Button regions for click detection
        self.button_regions = {}
        
        # Font
        self.font_small = font.Font(family="Arial", size=8)
        self.font_medium = font.Font(family="Arial", size=12)
        self.font_large = font.Font(family="Arial", size=16)
        self.font_xlarge = font.Font(family="Arial", size=24)
        
        # Bind click events
        self.canvas.bind("<Button-1>", self.on_click)
        self.canvas.bind("<ButtonRelease-1>", self.on_release)
        
        # Draw UI
        self.draw_main_screen()
        
        # Update loop
        self.root.after(100, self.update_loop)
    
    def draw_rect(self, x, y, w, h, color):
        """Draw rectangle"""
        r, g, b = rgb565_to_rgb(color)
        self.canvas.create_rectangle(
            x * self.scale, y * self.scale,
            (x + w) * self.scale, (y + h) * self.scale,
            fill=rgb_to_hex(r, g, b),
            outline=""
        )
    
    def draw_rect_outline(self, x, y, w, h, color):
        """Draw rectangle outline"""
        r, g, b = rgb565_to_rgb(color)
        self.canvas.create_rectangle(
            x * self.scale, y * self.scale,
            (x + w) * self.scale, (y + h) * self.scale,
            fill="",
            outline=rgb_to_hex(r, g, b),
            width=1
        )
    
    def draw_text(self, x, y, text, color, size=1):
        """Draw text"""
        r, g, b = rgb565_to_rgb(color)
        font_size = int(8 * size * self.scale)
        self.canvas.create_text(
            x * self.scale, y * self.scale,
            text=text,
            fill=rgb_to_hex(r, g, b),
            font=("Arial", font_size),
            anchor="nw"
        )
    
    def draw_main_screen(self, full_redraw=True):
        """Draw main screen UI"""
        if full_redraw:
            # Clear screen
            self.canvas.delete("all")
            self.button_regions.clear()
            # Draw background
            self.draw_rect(0, 0, 320, 240, self.COLOR_BG_DARK)
        
        # Başlıq paneli (0, 0, 320, 22)
        self.draw_rect(0, 0, 320, 22, self.COLOR_ACCENT_BLUE)
        self.draw_text(5, 5, "<M", self.COLOR_ACCENT_YELLOW, 1)
        self.draw_text(80, 5, "VALEH HPC", self.COLOR_TEXT_WHITE, 1)
        self.draw_text(170, 5, "[RST]", self.COLOR_ACCENT_GREEN, 1)
        self.draw_text(285, 5, f"M{self.coord_mode}>", self.COLOR_ACCENT_YELLOW, 1)
        
        # Sol panel - PRESET (5, 25, 105, 95)
        self.draw_rect(5, 25, 105, 95, self.COLOR_BG_PANEL)
        self.draw_rect_outline(5, 25, 105, 95, self.COLOR_ACCENT_BLUE)
        self.draw_rect(6, 26, 103, 16, self.COLOR_ACCENT_BLUE)
        self.draw_text(30, 28, "PRESET", self.COLOR_TEXT_WHITE, 1)
        
        # Preset düymələri (3x2 grid)
        for i in range(6):
            btn_x = 8 + (i % 3) * 34
            btn_y = 42 + (i // 3) * 38
            bg = self.preset_colors[i] if i == self.current_preset else self.COLOR_BG_DARK
            fg = self.COLOR_BG_DARK if i == self.current_preset else self.preset_colors[i]
            
            self.draw_rect(btn_x, btn_y, 32, 35, bg)
            self.draw_rect_outline(btn_x, btn_y, 32, 35, self.preset_colors[i])
            
            # Preset dəyəri
            val_text = f"{int(self.presets[i])}"
            self.draw_text(btn_x + 8, btn_y + 5, val_text, fg, 1)
            self.draw_text(btn_x + 2, btn_y + 20, self.preset_names[i], fg, 1)
            
            # Register button region for click detection
            self.button_regions[f"preset_{i}"] = {
                "x": btn_x * self.scale,
                "y": btn_y * self.scale,
                "w": 32 * self.scale,
                "h": 35 * self.scale,
                "action": lambda idx=i: self.on_preset_click(idx)
            }
        
        # Sağ panel - STATUS (210, 25, 105, 95)
        self.draw_rect(210, 25, 105, 95, self.COLOR_BG_PANEL)
        self.draw_rect_outline(210, 25, 105, 95, self.COLOR_ACCENT_BLUE)
        self.draw_rect(211, 26, 103, 16, self.COLOR_ACCENT_BLUE)
        self.draw_text(240, 28, "STATUS", self.COLOR_TEXT_WHITE, 1)
        
        # Status dəyərləri
        self.draw_text(215, 45, f"MTR:{self.motor_pwm:5.1f}%", self.COLOR_ACCENT_YELLOW, 1)
        self.draw_text(215, 60, f"ZME:{self.zme_pwm:5.1f}%", self.COLOR_CYAN, 1)
        self.draw_text(215, 75, f"DRV:{self.drv_pwm:5.1f}%", self.COLOR_ACCENT_GREEN, 1)
        err_color = self.COLOR_ACCENT_GREEN if abs(self.error) < 3.0 else self.COLOR_ACCENT_YELLOW
        self.draw_text(215, 95, f"ERR:{self.error:+5.1f}", err_color, 1)
        
        # Mərkəz - Pressure göstəricisi (115, 30, 90, 50)
        press_color = self.COLOR_ACCENT_GREEN if abs(self.current_pressure - self.target_pressure) < 5.0 else self.COLOR_ACCENT_YELLOW
        self.draw_text(115, 35, f"{self.current_pressure:.1f}", press_color, 3)
        self.draw_text(115, 65, "BAR", self.COLOR_TEXT_GREY, 1)
        
        # Hədəf göstəricisi
        self.draw_text(115, 80, f"SP: {int(self.target_pressure)} bar", self.COLOR_CYAN, 1)
        
        # Progress bar (115, 100, 90, 15)
        self.draw_rect_outline(115, 100, 90, 15, self.COLOR_BORDER)
        self.draw_rect(116, 101, 88, 13, self.COLOR_BG_DARK)
        percent = min(self.current_pressure / 300.0, 1.0)
        bar_width = int(86 * percent)
        if bar_width > 0:
            self.draw_rect(117, 102, bar_width, 11, press_color)
        
        # Gauge (160, 155, 35)
        self.draw_gauge(160, 155, 35, self.current_pressure, 300.0)
        
        # Alt panel - CONTROL (5, 195, 310, 42)
        self.draw_rect(5, 195, 310, 42, self.COLOR_BG_PANEL)
        self.draw_rect_outline(5, 195, 310, 42, self.COLOR_ACCENT_BLUE)
        self.draw_rect(6, 196, 308, 16, self.COLOR_ACCENT_BLUE)
        self.draw_text(140, 198, "CONTROL", self.COLOR_TEXT_WHITE, 1)
        
        # Control düymələri
        # START/STOP
        btn_color = self.COLOR_ACCENT_RED if self.system_running else self.COLOR_ACCENT_GREEN
        btn_text = "STOP" if self.system_running else "START"
        self.draw_rect(10, 208, 70, 25, btn_color)
        self.draw_rect_outline(10, 208, 70, 25, self.COLOR_BORDER)
        self.draw_text(35, 215, btn_text, self.COLOR_TEXT_WHITE, 1)
        self.button_regions["start_stop"] = {
            "x": 10 * self.scale, "y": 208 * self.scale,
            "w": 70 * self.scale, "h": 25 * self.scale,
            "action": self.on_start_stop_click
        }
        
        # MENU
        self.draw_rect(85, 208, 70, 25, COLOR_ACCENT_BLUE)
        self.draw_rect_outline(85, 208, 70, 25, COLOR_BORDER)
        self.draw_text(115, 215, "MENU", COLOR_TEXT_WHITE, 1)
        self.button_regions["menu"] = {
            "x": 85 * self.scale, "y": 208 * self.scale,
            "w": 70 * self.scale, "h": 25 * self.scale,
            "action": self.on_menu_click
        }
        
        # SP-
        self.draw_rect(160, 208, 35, 25, COLOR_BG_PANEL)
        self.draw_rect_outline(160, 208, 35, 25, COLOR_BORDER)
        self.draw_text(175, 215, "-", COLOR_TEXT_WHITE, 2)
        self.button_regions["sp_minus"] = {
            "x": 160 * self.scale, "y": 208 * self.scale,
            "w": 35 * self.scale, "h": 25 * self.scale,
            "action": self.on_sp_minus_click
        }
        
        # SP+
        self.draw_rect(200, 208, 35, 25, COLOR_BG_PANEL)
        self.draw_rect_outline(200, 208, 35, 25, COLOR_BORDER)
        self.draw_text(215, 215, "+", COLOR_TEXT_WHITE, 2)
        self.button_regions["sp_plus"] = {
            "x": 200 * self.scale, "y": 208 * self.scale,
            "w": 35 * self.scale, "h": 25 * self.scale,
            "action": self.on_sp_plus_click
        }
        
        # PID indicator
        pid_text = "PID:ON" if self.control_enabled else "PID:OFF"
        pid_color = COLOR_ACCENT_GREEN if self.control_enabled else COLOR_ACCENT_RED
        self.draw_text(245, 212, pid_text, pid_color, 1)
        
        # Kalibrasiya statusu (5, 175, 100, 18)
        self.draw_text(5, 175, "CAL OK(+0,+0)", COLOR_ACCENT_GREEN, 1)
        
        # Touch debug paneli (210, 122, 105, 70)
        self.draw_rect(210, 122, 105, 70, COLOR_BG_PANEL)
        self.draw_rect_outline(210, 122, 105, 70, 0xF81F)  # Magenta
        self.draw_text(215, 124, "TOUCH DEBUG", 0xF81F, 1)
        self.draw_text(213, 136, "XY:160,120", COLOR_TEXT_WHITE, 1)
        self.draw_text(213, 148, "RW:2048,2048", COLOR_CYAN, 1)
        self.draw_text(213, 160, "BT:---,---", COLOR_TEXT_GREY, 1)
        self.draw_text(213, 172, "DF:--,--", COLOR_TEXT_GREY, 1)
        self.draw_text(213, 184, f"M{self.coord_mode} B00", COLOR_ACCENT_YELLOW, 1)
    
    def draw_gauge(self, cx, cy, r, value, max_val):
        """Draw circular gauge"""
        percent = min(value / max_val, 1.0)
        start_angle = -135
        end_angle = -135 + int(percent * 270)
        
        # Gauge color
        if percent < 0.33:
            gauge_color = COLOR_ACCENT_GREEN
        elif percent < 0.66:
            gauge_color = COLOR_ACCENT_YELLOW
        else:
            gauge_color = COLOR_ACCENT_RED
        
        # Draw gauge arc (simplified)
        for angle in range(start_angle, end_angle, 3):
            rad = math.radians(angle)
            x1 = cx + (r - 6) * math.cos(rad)
            y1 = cy + (r - 6) * math.sin(rad)
            x2 = cx + (r - 10) * math.cos(rad)
            y2 = cy + (r - 10) * math.sin(rad)
            
            r_val, g_val, b_val = rgb565_to_rgb(gauge_color)
            self.canvas.create_line(
                x1 * self.scale, y1 * self.scale,
                x2 * self.scale, y2 * self.scale,
                fill=rgb_to_hex(r_val, g_val, b_val),
                width=2
            )
    
    def draw_menu_screen(self, full_redraw=True):
        """Draw menu screen UI"""
        if full_redraw:
            # Clear screen
            self.canvas.delete("all")
            self.button_regions.clear()
            # Draw background
            self.draw_rect(0, 0, 320, 240, self.COLOR_BG_DARK)
        
        # Başlıq (0, 0, 320, 30)
        self.draw_rect(0, 0, 320, 30, self.COLOR_ACCENT_BLUE)
        
        # Sol üst - mode azalt
        self.draw_text(5, 8, "<", self.COLOR_ACCENT_YELLOW, 2)
        
        # Mərkəz - MENU başlığı
        self.draw_text(130, 8, "MENU", self.COLOR_TEXT_WHITE, 2)
        
        # Sağ üst - mode artır
        self.draw_text(280, 8, f"M{self.coord_mode}>", self.COLOR_ACCENT_YELLOW, 1)
        
        # Menyu düymələri (C kodundan: Screen_DrawMenu)
        # SETPOINT: (40, 45) ölçü (240, 35)
        self.draw_rect(40, 45, 240, 35, self.COLOR_BG_PANEL)
        self.draw_rect_outline(40, 45, 240, 35, self.COLOR_BORDER)
        self.draw_text(120, 55, "SETPOINT", self.COLOR_CYAN, 2)
        self.button_regions["menu_setpoint"] = {
            "x": 40 * self.scale, "y": 45 * self.scale,
            "w": 240 * self.scale, "h": 35 * self.scale,
            "action": self.on_menu_setpoint_click
        }
        
        # PID TUNE: (40, 90) ölçü (240, 35)
        self.draw_rect(40, 90, 240, 35, self.COLOR_BG_PANEL)
        self.draw_rect_outline(40, 90, 240, 35, self.COLOR_BORDER)
        self.draw_text(120, 100, "PID TUNE", self.COLOR_ACCENT_YELLOW, 2)
        self.button_regions["menu_pid_tune"] = {
            "x": 40 * self.scale, "y": 90 * self.scale,
            "w": 240 * self.scale, "h": 35 * self.scale,
            "action": self.on_menu_pid_tune_click
        }
        
        # CALIBRATION: (40, 135) ölçü (240, 35)
        self.draw_rect(40, 135, 240, 35, self.COLOR_BG_PANEL)
        self.draw_rect_outline(40, 135, 240, 35, self.COLOR_BORDER)
        self.draw_text(110, 145, "CALIBRATION", self.COLOR_ACCENT_GREEN, 2)
        self.button_regions["menu_calibration"] = {
            "x": 40 * self.scale, "y": 135 * self.scale,
            "w": 240 * self.scale, "h": 35 * self.scale,
            "action": self.on_menu_calibration_click
        }
        
        # BACK: (40, 180) ölçü (240, 35)
        self.draw_rect(40, 180, 240, 35, self.COLOR_ACCENT_RED)
        self.draw_rect_outline(40, 180, 240, 35, self.COLOR_BORDER)
        self.draw_text(145, 190, "BACK", self.COLOR_TEXT_WHITE, 2)
        self.button_regions["menu_back"] = {
            "x": 40 * self.scale, "y": 180 * self.scale,
            "w": 240 * self.scale, "h": 35 * self.scale,
            "action": self.on_menu_back_click
        }
    
    def on_menu_setpoint_click(self):
        """Handle SETPOINT menu button click"""
        print("SETPOINT menu clicked")
        self.current_page = "setpoint"
        self.draw_setpoint_screen(full_redraw=True)
    
    def on_menu_pid_tune_click(self):
        """Handle PID TUNE menu button click"""
        print("PID TUNE menu clicked")
        self.current_page = "pid_tune"
        self.draw_pid_tune_screen(full_redraw=True)
    
    def on_menu_calibration_click(self):
        """Handle CALIBRATION menu button click"""
        print("CALIBRATION menu clicked")
        self.current_page = "calibration"
        self.draw_calibration_screen(full_redraw=True)
    
    def on_menu_back_click(self):
        """Handle BACK menu button click"""
        print("BACK menu clicked - returning to main screen")
        self.current_page = "main"
        self.draw_main_screen(full_redraw=True)
    
    def draw_setpoint_screen(self, full_redraw=True):
        """Draw setpoint screen UI"""
        if full_redraw:
            # Clear screen
            self.canvas.delete("all")
            self.button_regions.clear()
            # Draw background
            self.draw_rect(0, 0, 320, 240, self.COLOR_BG_DARK)
        
        # Başlıq (0, 0, 320, 30)
        self.draw_rect(0, 0, 320, 30, self.COLOR_ACCENT_BLUE)
        self.draw_text(80, 8, "SET PRESSURE", self.COLOR_TEXT_WHITE, 2)
        
        # Panel (20, 40, 280, 100) - "TARGET PRESSURE"
        self.draw_rect(20, 40, 280, 100, self.COLOR_BG_PANEL)
        self.draw_rect_outline(20, 40, 280, 100, self.COLOR_ACCENT_BLUE)
        self.draw_rect(21, 41, 278, 16, self.COLOR_ACCENT_BLUE)
        self.draw_text(100, 43, "TARGET PRESSURE", self.COLOR_TEXT_WHITE, 1)
        
        # Hazırki dəyər (100, 60, 120, 30)
        val_str = f"{int(self.target_pressure)} BAR"
        self.draw_rect(100, 60, 120, 30, self.COLOR_BG_PANEL)
        self.draw_text(110, 65, val_str, self.COLOR_ACCENT_YELLOW, 2)
        
        # Düymələr (25, 100, 50, 35)
        # -10
        self.draw_rect(25, 100, 50, 35, self.COLOR_ACCENT_RED)
        self.draw_rect_outline(25, 100, 50, 35, self.COLOR_BORDER)
        self.draw_text(35, 110, "-10", self.COLOR_TEXT_WHITE, 2)
        self.button_regions["sp_minus_10"] = {
            "x": 25 * self.scale, "y": 100 * self.scale,
            "w": 50 * self.scale, "h": 35 * self.scale,
            "action": lambda: self.on_sp_change(-10)
        }
        
        # -1
        self.draw_rect(80, 100, 50, 35, self.COLOR_ACCENT_YELLOW)
        self.draw_rect_outline(80, 100, 50, 35, self.COLOR_BORDER)
        self.draw_text(95, 110, "-1", self.COLOR_BG_DARK, 2)
        self.button_regions["sp_minus_1"] = {
            "x": 80 * self.scale, "y": 100 * self.scale,
            "w": 50 * self.scale, "h": 35 * self.scale,
            "action": lambda: self.on_sp_change(-1)
        }
        
        # +1
        self.draw_rect(190, 100, 50, 35, self.COLOR_ACCENT_GREEN)
        self.draw_rect_outline(190, 100, 50, 35, self.COLOR_BORDER)
        self.draw_text(205, 110, "+1", self.COLOR_BG_DARK, 2)
        self.button_regions["sp_plus_1"] = {
            "x": 190 * self.scale, "y": 100 * self.scale,
            "w": 50 * self.scale, "h": 35 * self.scale,
            "action": lambda: self.on_sp_change(1)
        }
        
        # +10
        self.draw_rect(245, 100, 50, 35, self.COLOR_ACCENT_GREEN)
        self.draw_rect_outline(245, 100, 50, 35, self.COLOR_BORDER)
        self.draw_text(255, 110, "+10", self.COLOR_TEXT_WHITE, 2)
        self.button_regions["sp_plus_10"] = {
            "x": 245 * self.scale, "y": 100 * self.scale,
            "w": 50 * self.scale, "h": 35 * self.scale,
            "action": lambda: self.on_sp_change(10)
        }
        
        # Preset düymələri (25 + i * 48, 150, 45, 25)
        for i in range(6):
            btn_x = 25 + i * 48
            self.draw_rect(btn_x, 150, 45, 25, self.preset_colors[i])
            self.draw_rect_outline(btn_x, 150, 45, 25, self.COLOR_BORDER)
            self.draw_text(btn_x + 5, 157, self.preset_names[i], self.COLOR_BG_DARK, 1)
            self.button_regions[f"sp_preset_{i}"] = {
                "x": btn_x * self.scale, "y": 150 * self.scale,
                "w": 45 * self.scale, "h": 25 * self.scale,
                "action": lambda idx=i: self.on_sp_preset_click(idx)
            }
        
        # BACK düyməsi (100, 190, 120, 35)
        self.draw_rect(100, 190, 120, 35, self.COLOR_ACCENT_BLUE)
        self.draw_rect_outline(100, 190, 120, 35, self.COLOR_BORDER)
        self.draw_text(145, 200, "BACK", self.COLOR_TEXT_WHITE, 2)
        self.button_regions["setpoint_back"] = {
            "x": 100 * self.scale, "y": 190 * self.scale,
            "w": 120 * self.scale, "h": 35 * self.scale,
            "action": self.on_setpoint_back_click
        }
    
    def draw_pid_tune_screen(self, full_redraw=True):
        """Draw PID tune screen UI"""
        if full_redraw:
            # Clear screen
            self.canvas.delete("all")
            self.button_regions.clear()
            # Draw background
            self.draw_rect(0, 0, 320, 240, self.COLOR_BG_DARK)
        
        # Başlıq (0, 0, 320, 30)
        self.draw_rect(0, 0, 320, 30, self.COLOR_ACCENT_BLUE)
        self.draw_text(100, 8, "PID TUNING", self.COLOR_TEXT_WHITE, 2)
        
        # Kp sırası (20, 45)
        self.draw_text(20, 45, "Kp:", self.COLOR_TEXT_WHITE, 2)
        self.draw_text(80, 45, f"{self.pid_kp:.3f}", self.COLOR_ACCENT_GREEN, 2)
        # Kp-
        self.draw_rect(200, 40, 40, 30, self.COLOR_BG_PANEL)
        self.draw_rect_outline(200, 40, 40, 30, self.COLOR_BORDER)
        self.draw_text(215, 48, "-", self.COLOR_TEXT_WHITE, 2)
        self.button_regions["pid_kp_minus"] = {
            "x": 200 * self.scale, "y": 40 * self.scale,
            "w": 40 * self.scale, "h": 30 * self.scale,
            "action": lambda: self.on_pid_change("kp", -0.05)
        }
        # Kp+
        self.draw_rect(260, 40, 40, 30, self.COLOR_BG_PANEL)
        self.draw_rect_outline(260, 40, 40, 30, self.COLOR_BORDER)
        self.draw_text(275, 48, "+", self.COLOR_TEXT_WHITE, 2)
        self.button_regions["pid_kp_plus"] = {
            "x": 260 * self.scale, "y": 40 * self.scale,
            "w": 40 * self.scale, "h": 30 * self.scale,
            "action": lambda: self.on_pid_change("kp", 0.05)
        }
        
        # Ki sırası (20, 85)
        self.draw_text(20, 85, "Ki:", self.COLOR_TEXT_WHITE, 2)
        self.draw_text(80, 85, f"{self.pid_ki:.4f}", self.COLOR_ACCENT_GREEN, 2)
        # Ki-
        self.draw_rect(200, 80, 40, 30, self.COLOR_BG_PANEL)
        self.draw_rect_outline(200, 80, 40, 30, self.COLOR_BORDER)
        self.draw_text(215, 88, "-", self.COLOR_TEXT_WHITE, 2)
        self.button_regions["pid_ki_minus"] = {
            "x": 200 * self.scale, "y": 80 * self.scale,
            "w": 40 * self.scale, "h": 30 * self.scale,
            "action": lambda: self.on_pid_change("ki", -0.005)
        }
        # Ki+
        self.draw_rect(260, 80, 40, 30, self.COLOR_BG_PANEL)
        self.draw_rect_outline(260, 80, 40, 30, self.COLOR_BORDER)
        self.draw_text(275, 88, "+", self.COLOR_TEXT_WHITE, 2)
        self.button_regions["pid_ki_plus"] = {
            "x": 260 * self.scale, "y": 80 * self.scale,
            "w": 40 * self.scale, "h": 30 * self.scale,
            "action": lambda: self.on_pid_change("ki", 0.005)
        }
        
        # Kd sırası (20, 125)
        self.draw_text(20, 125, "Kd:", self.COLOR_TEXT_WHITE, 2)
        self.draw_text(80, 125, f"{self.pid_kd:.3f}", self.COLOR_ACCENT_GREEN, 2)
        # Kd-
        self.draw_rect(200, 120, 40, 30, self.COLOR_BG_PANEL)
        self.draw_rect_outline(200, 120, 40, 30, self.COLOR_BORDER)
        self.draw_text(215, 128, "-", self.COLOR_TEXT_WHITE, 2)
        self.button_regions["pid_kd_minus"] = {
            "x": 200 * self.scale, "y": 120 * self.scale,
            "w": 40 * self.scale, "h": 30 * self.scale,
            "action": lambda: self.on_pid_change("kd", -0.01)
        }
        # Kd+
        self.draw_rect(260, 120, 40, 30, self.COLOR_BG_PANEL)
        self.draw_rect_outline(260, 120, 40, 30, self.COLOR_BORDER)
        self.draw_text(275, 128, "+", self.COLOR_TEXT_WHITE, 2)
        self.button_regions["pid_kd_plus"] = {
            "x": 260 * self.scale, "y": 120 * self.scale,
            "w": 40 * self.scale, "h": 30 * self.scale,
            "action": lambda: self.on_pid_change("kd", 0.01)
        }
        
        # SP sırası (20, 165)
        self.draw_text(20, 165, "SP:", self.COLOR_TEXT_WHITE, 2)
        self.draw_text(80, 165, f"{int(self.target_pressure)} bar", self.COLOR_ACCENT_YELLOW, 2)
        # SP-
        self.draw_rect(200, 160, 40, 30, self.COLOR_BG_PANEL)
        self.draw_rect_outline(200, 160, 40, 30, self.COLOR_BORDER)
        self.draw_text(215, 168, "-", self.COLOR_TEXT_WHITE, 2)
        self.button_regions["pid_sp_minus"] = {
            "x": 200 * self.scale, "y": 160 * self.scale,
            "w": 40 * self.scale, "h": 30 * self.scale,
            "action": lambda: self.on_pid_change("sp", -10)
        }
        # SP+
        self.draw_rect(260, 160, 40, 30, self.COLOR_BG_PANEL)
        self.draw_rect_outline(260, 160, 40, 30, self.COLOR_BORDER)
        self.draw_text(275, 168, "+", self.COLOR_TEXT_WHITE, 2)
        self.button_regions["pid_sp_plus"] = {
            "x": 260 * self.scale, "y": 160 * self.scale,
            "w": 40 * self.scale, "h": 30 * self.scale,
            "action": lambda: self.on_pid_change("sp", 10)
        }
        
        # BACK düyməsi (20, 200, 80, 30)
        self.draw_rect(20, 200, 80, 30, self.COLOR_ACCENT_BLUE)
        self.draw_rect_outline(20, 200, 80, 30, self.COLOR_BORDER)
        self.draw_text(55, 210, "BACK", self.COLOR_TEXT_WHITE, 1)
        self.button_regions["pid_back"] = {
            "x": 20 * self.scale, "y": 200 * self.scale,
            "w": 80 * self.scale, "h": 30 * self.scale,
            "action": self.on_pid_back_click
        }
        
        # SAVE düyməsi (220, 200, 80, 30)
        self.draw_rect(220, 200, 80, 30, self.COLOR_ACCENT_GREEN)
        self.draw_rect_outline(220, 200, 80, 30, self.COLOR_BORDER)
        self.draw_text(250, 210, "SAVE", self.COLOR_TEXT_WHITE, 1)
        self.button_regions["pid_save"] = {
            "x": 220 * self.scale, "y": 200 * self.scale,
            "w": 80 * self.scale, "h": 30 * self.scale,
            "action": self.on_pid_save_click
        }
    
    def draw_calibration_screen(self, full_redraw=True):
        """Draw calibration screen UI"""
        if full_redraw:
            # Clear screen
            self.canvas.delete("all")
            self.button_regions.clear()
            # Draw background
            self.draw_rect(0, 0, 320, 240, self.COLOR_BG_DARK)
        
        # Başlıq (0, 0, 320, 30)
        self.draw_rect(0, 0, 320, 30, self.COLOR_ACCENT_BLUE)
        self.draw_text(80, 8, "CALIBRATION", self.COLOR_TEXT_WHITE, 2)
        
        # CAL MIN düyməsi (20, 60, 130, 40)
        self.draw_rect(20, 60, 130, 40, self.COLOR_ACCENT_YELLOW)
        self.draw_rect_outline(20, 60, 130, 40, self.COLOR_BORDER)
        self.draw_text(60, 75, "CAL MIN", self.COLOR_BG_DARK, 2)
        self.button_regions["cal_min"] = {
            "x": 20 * self.scale, "y": 60 * self.scale,
            "w": 130 * self.scale, "h": 40 * self.scale,
            "action": self.on_cal_min_click
        }
        
        # CAL MAX düyməsi (170, 60, 130, 40)
        self.draw_rect(170, 60, 130, 40, self.COLOR_ACCENT_GREEN)
        self.draw_rect_outline(170, 60, 130, 40, self.COLOR_BORDER)
        self.draw_text(210, 75, "CAL MAX", self.COLOR_TEXT_WHITE, 2)
        self.button_regions["cal_max"] = {
            "x": 170 * self.scale, "y": 60 * self.scale,
            "w": 130 * self.scale, "h": 40 * self.scale,
            "action": self.on_cal_max_click
        }
        
        # ADC və Pressure dəyərləri (20, 115, 280, 55)
        self.draw_rect(20, 115, 280, 55, self.COLOR_BG_DARK)
        self.draw_text(30, 120, f"ADC:  {self.raw_adc_value}", self.COLOR_ACCENT_YELLOW, 2)
        self.draw_text(30, 145, f"PRES: {self.current_pressure:.2f} bar", self.COLOR_ACCENT_GREEN, 2)
        
        # BACK düyməsi (20, 180, 100, 35)
        self.draw_rect(20, 180, 100, 35, self.COLOR_ACCENT_BLUE)
        self.draw_rect_outline(20, 180, 100, 35, self.COLOR_BORDER)
        self.draw_text(55, 190, "BACK", self.COLOR_TEXT_WHITE, 2)
        self.button_regions["cal_back"] = {
            "x": 20 * self.scale, "y": 180 * self.scale,
            "w": 100 * self.scale, "h": 35 * self.scale,
            "action": self.on_cal_back_click
        }
        
        # SAVE düyməsi (200, 180, 100, 35)
        self.draw_rect(200, 180, 100, 35, self.COLOR_ACCENT_GREEN)
        self.draw_rect_outline(200, 180, 100, 35, self.COLOR_BORDER)
        self.draw_text(235, 190, "SAVE", self.COLOR_TEXT_WHITE, 2)
        self.button_regions["cal_save"] = {
            "x": 200 * self.scale, "y": 180 * self.scale,
            "w": 100 * self.scale, "h": 35 * self.scale,
            "action": self.on_cal_save_click
        }
    
    def on_click(self, event):
        """Handle mouse click"""
        x, y = event.x, event.y
        
        # Check all button regions
        for btn_name, btn_info in self.button_regions.items():
            if (btn_info["x"] <= x <= btn_info["x"] + btn_info["w"] and
                btn_info["y"] <= y <= btn_info["y"] + btn_info["h"]):
                # Button clicked!
                if callable(btn_info["action"]):
                    btn_info["action"]()
                return
    
    def on_release(self, event):
        """Handle mouse release"""
        pass
    
    def on_preset_click(self, index):
        """Handle preset button click"""
        print(f"Preset {index} clicked: {self.presets[index]} bar")
        self.current_preset = index
        self.target_pressure = self.presets[index]
        self.draw_main_screen(full_redraw=True)
    
    def on_start_stop_click(self):
        """Handle START/STOP button click"""
        self.system_running = not self.system_running
        print(f"System {'STARTED' if self.system_running else 'STOPPED'}")
        self.draw_main_screen(full_redraw=True)
    
    def on_menu_click(self):
        """Handle MENU button click"""
        print("MENU clicked")
        self.current_page = "menu"
        self.draw_menu_screen(full_redraw=True)
    
    def on_sp_minus_click(self):
        """Handle SP- button click"""
        self.target_pressure = max(0, self.target_pressure - 5)
        print(f"Setpoint decreased to {self.target_pressure} bar")
        self.draw_main_screen(full_redraw=True)
    
    def on_sp_plus_click(self):
        """Handle SP+ button click"""
        self.target_pressure = min(300, self.target_pressure + 5)
        print(f"Setpoint increased to {self.target_pressure} bar")
        self.draw_main_screen(full_redraw=True)
    
    # Setpoint screen handlers
    def on_sp_change(self, delta):
        """Handle setpoint change in setpoint screen"""
        self.target_pressure = max(0, min(300, self.target_pressure + delta))
        print(f"Setpoint changed to {self.target_pressure} bar")
        self.draw_setpoint_screen(full_redraw=True)
    
    def on_sp_preset_click(self, index):
        """Handle preset button click in setpoint screen"""
        self.target_pressure = self.presets[index]
        self.current_preset = index
        print(f"Preset {index} selected: {self.target_pressure} bar")
        self.draw_setpoint_screen(full_redraw=True)
    
    def on_setpoint_back_click(self):
        """Handle BACK button in setpoint screen"""
        print("Setpoint BACK clicked - returning to menu")
        self.current_page = "menu"
        self.draw_menu_screen(full_redraw=True)
    
    # PID tune screen handlers
    def on_pid_change(self, param, delta):
        """Handle PID parameter change"""
        if param == "kp":
            self.pid_kp = max(0.0, min(10.0, self.pid_kp + delta))
            print(f"Kp changed to {self.pid_kp:.3f}")
        elif param == "ki":
            self.pid_ki = max(0.0, min(1.0, self.pid_ki + delta))
            print(f"Ki changed to {self.pid_ki:.4f}")
        elif param == "kd":
            self.pid_kd = max(0.0, min(5.0, self.pid_kd + delta))
            print(f"Kd changed to {self.pid_kd:.3f}")
        elif param == "sp":
            self.target_pressure = max(0, min(300, self.target_pressure + delta))
            print(f"SP changed to {self.target_pressure} bar")
        self.draw_pid_tune_screen(full_redraw=True)
    
    def on_pid_back_click(self):
        """Handle BACK button in PID tune screen"""
        print("PID BACK clicked - returning to menu")
        self.current_page = "menu"
        self.draw_menu_screen(full_redraw=True)
    
    def on_pid_save_click(self):
        """Handle SAVE button in PID tune screen"""
        print(f"PID parameters saved: Kp={self.pid_kp:.3f}, Ki={self.pid_ki:.4f}, Kd={self.pid_kd:.3f}")
        # In real system, this would save to flash
        self.draw_pid_tune_screen(full_redraw=True)
    
    # Calibration screen handlers
    def on_cal_min_click(self):
        """Handle CAL MIN button click"""
        print(f"CAL MIN clicked: ADC={self.raw_adc_value}")
        self.draw_calibration_screen(full_redraw=True)
    
    def on_cal_max_click(self):
        """Handle CAL MAX button click"""
        print(f"CAL MAX clicked: ADC={self.raw_adc_value}")
        self.draw_calibration_screen(full_redraw=True)
    
    def on_cal_back_click(self):
        """Handle BACK button in calibration screen"""
        print("Calibration BACK clicked - returning to menu")
        self.current_page = "menu"
        self.draw_menu_screen(full_redraw=True)
    
    def on_cal_save_click(self):
        """Handle SAVE button in calibration screen"""
        print("Calibration saved")
        # In real system, this would save to flash
        self.draw_calibration_screen(full_redraw=True)
    
    def update_loop(self):
        """Update loop - simulate pressure changes"""
        # Simulate pressure changes
        if self.system_running:
            # Pressure slowly approaches target
            diff = self.target_pressure - self.current_pressure
            self.current_pressure += diff * 0.1
            
            # Update PWM values according to new logic
            # Motor: 50 bar-da 10%, 300 bar-da 50% (linear interpolation)
            if self.target_pressure <= 50.0:
                # 0-50 bar: 5% to 10%
                self.motor_pwm = 5.0 + (self.target_pressure / 50.0) * 5.0
            else:
                # 50-300 bar: 10% to 50%
                self.motor_pwm = 10.0 + ((self.target_pressure - 50.0) / 250.0) * 40.0
            if self.motor_pwm > 50.0:
                self.motor_pwm = 50.0
            if self.motor_pwm < 5.0:
                self.motor_pwm = 5.0
            
            # ZME: 0% = tam açıq, 30% = tam bağlı (inverse logic)
            # Error müsbət (təzyiq aşağı) → ZME açılmalıdır (PWM azalır)
            # Error mənfi (təzyiq yüksək) → ZME bağlanmalıdır (PWM artır)
            zme_base = (self.target_pressure / 300.0) * 30.0  # 0-30% range
            if zme_base > 30.0:
                zme_base = 30.0
            # PID təshisi: diff müsbət → ZME açılır (azalır), diff mənfi → ZME bağlanır (artır)
            pid_trim = diff * 0.3  # Simplified PID
            self.zme_pwm = zme_base - pid_trim  # Inverse logic
            if self.zme_pwm < 0.0:
                self.zme_pwm = 0.0
            if self.zme_pwm > 30.0:
                self.zme_pwm = 30.0
            
            # DRV: 0-10% = tam açıq (dead zone), 10-40% = yavaş-yavaş bağlanır
            # Error müsbət (təzyiq aşağı) → DRV bağlanmalıdır (PWM artır)
            # Error mənfi (təzyiq yüksək) → DRV açılmalıdır (PWM azalır)
            # Baza: 0 bar → 10%, 300 bar → 40%
            drv_base = 10.0 + (self.target_pressure / 300.0) * 30.0  # 10-40% range
            if drv_base > 40.0:
                drv_base = 40.0
            if drv_base < 10.0:
                drv_base = 10.0
            # PID təshisi: diff müsbət → DRV bağlanır (artır), diff mənfi → DRV açılır (azalır)
            pid_trim = diff * 0.3  # Simplified PID
            self.drv_pwm = drv_base - pid_trim  # Inverse logic
            if self.drv_pwm < 0.0:
                self.drv_pwm = 0.0
            if self.drv_pwm > 40.0:
                self.drv_pwm = 40.0
            # QEYD: Display üçün real dəyəri göstəririk
            # Amma fiziki olaraq: drv_pwm < 10% → 0% kimi işləyir (tam açıq)
            
            self.error = diff
        
        # Simulate ADC value (based on pressure)
        # ADC range: 0-4095, pressure range: 0-300 bar
        self.raw_adc_value = int(2048 + (self.current_pressure - 100) * 10)
        if self.raw_adc_value < 0:
            self.raw_adc_value = 0
        if self.raw_adc_value > 4095:
            self.raw_adc_value = 4095
        
        # Redraw screen based on current page
        old_regions = self.button_regions.copy()
        if self.current_page == "menu":
            self.draw_menu_screen(full_redraw=True)
        elif self.current_page == "setpoint":
            self.draw_setpoint_screen(full_redraw=True)
        elif self.current_page == "pid_tune":
            self.draw_pid_tune_screen(full_redraw=True)
        elif self.current_page == "calibration":
            self.draw_calibration_screen(full_redraw=True)
        else:
            self.draw_main_screen(full_redraw=True)
            # Restore button regions after redraw
            for key, value in old_regions.items():
                if key in self.button_regions:
                    # Keep the action callback
                    self.button_regions[key]["action"] = value["action"]
        
        # Schedule next update
        self.root.after(100, self.update_loop)
    
    def update_dynamic_values(self):
        """Update only dynamic values - simplified version"""
        # Just redraw the screen (it's fast enough)
        # For better performance, we could cache button regions
        pass
    
    def run(self):
        """Run the GUI"""
        self.root.mainloop()

if __name__ == "__main__":
    print("VALEH Pressure Control UI Simulator")
    print("Proyektin UI kodlarını analiz edib virtual LCD-də göstərir")
    print("")
    
    simulator = VALEH_UISimulator(320, 240, scale=2)
    simulator.run()

