"""
C kodlarından UI konfiqurasiyasını parse edir
Bu, UI Simulator-un C kodları ilə sinxron qalmasını təmin edir
"""

import re
import os

def parse_c_defines(c_file_path):
    """C faylından #define-ləri parse edir"""
    defines = {}
    
    if not os.path.exists(c_file_path):
        print(f"Warning: {c_file_path} not found")
        return defines
    
    with open(c_file_path, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    
    # #define pattern: #define NAME value
    pattern = r'#define\s+(\w+)\s+(.+?)(?:\n|$)'
    matches = re.findall(pattern, content, re.MULTILINE)
    
    for name, value in matches:
        value = value.strip()
        # Remove comments
        if '//' in value:
            value = value.split('//')[0].strip()
        if '/*' in value:
            value = value.split('/*')[0].strip()
        
        # Try to parse as number
        try:
            # Hex
            if value.startswith('0x') or value.startswith('0X'):
                defines[name] = int(value, 16)
            # Decimal
            elif value.isdigit() or (value.startswith('-') and value[1:].isdigit()):
                defines[name] = int(value)
            # Float
            elif '.' in value:
                defines[name] = float(value)
            else:
                defines[name] = value
        except:
            defines[name] = value
    
    return defines

def parse_ui_config(project_root):
    """Proyektin C kodlarından UI konfiqurasiyasını parse edir"""
    config = {
        'colors': {},
        'dimensions': {},
        'presets': [],
        'preset_names': []
    }
    
    main_c = os.path.join(project_root, 'Core', 'Src', 'main.c')
    defines = parse_c_defines(main_c)
    
    # Rənglər
    color_names = [
        'COLOR_BG_DARK', 'COLOR_BG_PANEL', 'COLOR_ACCENT_BLUE',
        'COLOR_ACCENT_GREEN', 'COLOR_ACCENT_RED', 'COLOR_ACCENT_YELLOW',
        'COLOR_ACCENT_ORANGE', 'COLOR_TEXT_WHITE', 'COLOR_TEXT_GREY',
        'COLOR_BORDER'
    ]
    
    for color_name in color_names:
        if color_name in defines:
            config['colors'][color_name] = defines[color_name]
    
    # Ölçülər
    if 'SCREEN_WIDTH' in defines:
        config['dimensions']['width'] = defines['SCREEN_WIDTH']
    if 'SCREEN_HEIGHT' in defines:
        config['dimensions']['height'] = defines['SCREEN_HEIGHT']
    
    # Preset dəyərləri - main.c-dən oxuyur
    try:
        with open(main_c, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
        
        # Preset array tap - static const float g_presets[NUM_PRESETS] = {...}
        preset_pattern = r'(?:static\s+)?(?:const\s+)?float\s+g_presets\[.*?\]\s*=\s*\{([^}]+)\}'
        match = re.search(preset_pattern, content, re.MULTILINE | re.DOTALL)
        if match:
            values_str = match.group(1)
            # Extract numbers (including floats with 'f' suffix)
            numbers = re.findall(r'(\d+\.?\d*)f?', values_str)
            config['presets'] = [float(n) for n in numbers[:6]]  # First 6
        
        # Preset names - static const char* g_preset_names[NUM_PRESETS] = {...}
        name_pattern = r'(?:static\s+)?const\s+char\s*\*\s*g_preset_names\[.*?\]\s*=\s*\{([^}]+)\}'
        match = re.search(name_pattern, content, re.MULTILINE | re.DOTALL)
        if match:
            names_str = match.group(1)
            # Extract strings
            names = re.findall(r'"([^"]+)"', names_str)
            config['preset_names'] = names[:6]  # First 6
            
    except Exception as e:
        print(f"Warning: Could not parse presets: {e}")
    
    return config, defines

if __name__ == "__main__":
    # Test
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    config, defines = parse_ui_config(project_root)
    
    print("UI Configuration from C code:")
    print(f"Colors: {config['colors']}")
    print(f"Dimensions: {config['dimensions']}")
    print(f"Presets: {config['presets']}")
    print(f"Preset Names: {config['preset_names']}")

