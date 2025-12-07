# STM32CubeIDE - Simulator Target Tapmaq

## 📍 Simulator Target Haradadır?

"Debugger" tab-ında **"Debug probe"** dropdown-ında olmalıdır!

## 🎯 Addım-Addım

### Addım 1: Debug probe Dropdown-ı Tapın

"Debugger" tab-ında (indi açıqdır):

1. **"Debug probe:"** bölməsini tapın
2. Hazırda **"ST-LINK (ST-LINK GDB server)"** seçilib
3. Bu dropdown-a klikləyin

### Addım 2: Simulator Seçin

Dropdown-da seçimlər görünəcək:
- ST-LINK (ST-LINK GDB server) ← Real hardware üçün
- **Simulator** ← **BUNU SEÇİN!**
- Və ya başqa seçimlər

### Addım 3: Simulator Seçildikdə

"Simulator" seçdikdən sonra:
- Bəzi parametrlər dəyişəcək
- "Interface" bölməsi gizlənə bilər (simulator üçün lazım deyil)
- "Apply" düyməsinə basın
- "Debug" düyməsinə basın

## ⚠️ Problem: Simulator Seçimi Görünmür?

Əgər "Debug probe" dropdown-da "Simulator" seçimi yoxdursa:

### Həll 1: STM32CubeIDE Versiyasını Yoxlayın

1. Help → About STM32CubeIDE
2. Versiyanı yoxlayın (1.13.0+ tövsiyə olunur)
3. Əgər köhnədirsə, yeniləyin

### Həll 2: Proyekt Konfiqurasiyasını Yoxlayın

1. Proyekt sağ klik → Properties
2. C/C++ Build → Settings → Tool Settings
3. MCU GCC Compiler → Target seçimini yoxlayın
4. STM32F407ZG olmalıdır

### Həll 3: Yeni Debug Konfiqurasiyası Yaradın

1. Debug Configurations pəncərəsində
2. Sol tərəfdə "STM32 C/C++ Application" sağ klik
3. "New Configuration" seçin
4. Yeni konfiqurasiyada "Debug probe" dropdown-da "Simulator" görünəcək

## 🔧 Alternativ: Manual Konfiqurasiya

Əgər "Simulator" seçimi hələ də görünmürsə:

1. "Debug probe" dropdown-da başqa seçimlər yoxlayın
2. Və ya "GDB Connection Settings" bölməsində dəyişiklik edin
3. "Autostart local GDB server" seçili olduğundan əmin olun

## 📝 Qeyd

Bəzi STM32CubeIDE versiyalarında "Simulator" seçimi:
- "Debug probe" dropdown-da olur
- Və ya ayrı "Target" bölməsində olur
- Və ya "GDB Connection Settings" altında olur

## 💡 Tövsiyə

1. "Debug probe" dropdown-ı açın və bütün seçimləri yoxlayın
2. Əgər "Simulator" yoxdursa, yeni konfiqurasiya yaradın
3. Və ya STM32CubeIDE-ni yeniləyin

