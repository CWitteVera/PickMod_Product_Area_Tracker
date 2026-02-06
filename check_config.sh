#!/bin/bash
# Configuration Verification Script
# Run this before building to ensure your configuration is correct

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo "=========================================="
echo "ESP32-S3 Display Configuration Checker"
echo "=========================================="
echo ""

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check 1: sdkconfig.defaults exists
echo -n "Checking sdkconfig.defaults exists... "
if [ -f "sdkconfig.defaults" ]; then
    echo -e "${GREEN}✓${NC}"
else
    echo -e "${RED}✗ MISSING${NC}"
    echo "ERROR: sdkconfig.defaults not found!"
    exit 1
fi

# Check 2: Critical PSRAM settings
echo -n "Checking PSRAM configuration... "
if grep -q "CONFIG_SPIRAM=y" sdkconfig.defaults && \
   grep -q "CONFIG_SPIRAM_MODE_OCT=y" sdkconfig.defaults && \
   grep -q "CONFIG_SPIRAM_BOOT_INIT=y" sdkconfig.defaults; then
    echo -e "${GREEN}✓${NC}"
else
    echo -e "${RED}✗ INCOMPLETE${NC}"
    echo "ERROR: PSRAM configuration missing or incomplete in sdkconfig.defaults"
    exit 1
fi

# Check 3: Flash size configuration
echo -n "Checking flash size configuration... "
if grep -q "CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y" sdkconfig.defaults; then
    echo -e "${GREEN}✓${NC}"
else
    echo -e "${YELLOW}⚠ WARNING${NC}"
    echo "WARNING: Flash size may not be configured for 16MB"
fi

# Check 4: Font configuration
echo -n "Checking Montserrat 24 font... "
if grep -q "CONFIG_LV_FONT_MONTSERRAT_24=y" sdkconfig.defaults; then
    echo -e "${GREEN}✓${NC}"
else
    echo -e "${RED}✗ NOT ENABLED${NC}"
    echo "ERROR: CONFIG_LV_FONT_MONTSERRAT_24 not enabled in sdkconfig.defaults"
    exit 1
fi

# Check 5: LCD configuration
echo -n "Checking LCD RGB configuration... "
if grep -q "CONFIG_LCD_RGB_ISR_IRAM_SAFE=y" sdkconfig.defaults && \
   grep -q "CONFIG_LCD_RGB_RESTART_IN_VSYNC=y" sdkconfig.defaults; then
    echo -e "${GREEN}✓${NC}"
else
    echo -e "${YELLOW}⚠ WARNING${NC}"
    echo "WARNING: LCD RGB configuration may be incomplete"
fi

# Check 6: Stale sdkconfig file
echo -n "Checking for stale sdkconfig... "
if [ -f "sdkconfig" ]; then
    # Check if sdkconfig is newer than sdkconfig.defaults
    if [ "sdkconfig" -ot "sdkconfig.defaults" ]; then
        echo -e "${YELLOW}⚠ STALE${NC}"
        echo "WARNING: sdkconfig exists and is older than sdkconfig.defaults"
        echo "         This may cause build issues. Consider deleting it."
    else
        # Check if it has the flash size configured correctly
        if grep -q "CONFIG_ESPTOOLPY_FLASHSIZE_2MB=y" sdkconfig 2>/dev/null; then
            echo -e "${RED}✗ INCORRECT${NC}"
            echo "ERROR: sdkconfig has wrong flash size (2MB instead of 16MB)"
            echo ""
            echo "SOLUTION: Delete the stale sdkconfig file:"
            echo "  rm -f sdkconfig sdkconfig.old"
            echo "  idf.py set-target esp32s3"
            exit 1
        elif ! grep -q "CONFIG_SPIRAM=y" sdkconfig 2>/dev/null; then
            echo -e "${RED}✗ MISSING PSRAM${NC}"
            echo "ERROR: sdkconfig is missing PSRAM configuration"
            echo ""
            echo "SOLUTION: Delete the stale sdkconfig file:"
            echo "  rm -f sdkconfig sdkconfig.old"
            echo "  idf.py set-target esp32s3"
            exit 1
        else
            echo -e "${GREEN}✓${NC}"
        fi
    fi
else
    echo -e "${GREEN}✓ (not present - will be generated)${NC}"
fi

# Check 7: ESP-IDF environment
echo -n "Checking ESP-IDF environment... "
if command -v idf.py &> /dev/null; then
    IDF_VERSION=$(idf.py --version 2>&1 | grep -oP 'v\d+\.\d+(\.\d+)?' | head -1)
    if [ ! -z "$IDF_VERSION" ]; then
        echo -e "${GREEN}✓ ($IDF_VERSION)${NC}"
        # Check if version is 5.2.x
        if [[ ! "$IDF_VERSION" =~ ^v5\.2 ]] && [[ ! "$IDF_VERSION" =~ ^v5\.5 ]]; then
            echo -e "${YELLOW}⚠ WARNING: ESP-IDF version is $IDF_VERSION${NC}"
            echo "           Recommended: v5.2.x or v5.5.x"
        fi
    else
        echo -e "${YELLOW}⚠ (version unknown)${NC}"
    fi
else
    echo -e "${RED}✗ NOT FOUND${NC}"
    echo "ERROR: ESP-IDF environment not activated"
    echo "       Run: . \$HOME/esp/esp-idf/export.sh"
    exit 1
fi

echo ""
echo "=========================================="
echo -e "${GREEN}Configuration Check PASSED${NC}"
echo "=========================================="
echo ""
echo "Your configuration appears correct!"
echo ""
echo "Next steps:"
echo "  1. If sdkconfig exists and is stale, delete it:"
echo "     rm -f sdkconfig sdkconfig.old"
echo ""
echo "  2. Set target (first time or after deleting sdkconfig):"
echo "     idf.py set-target esp32s3"
echo ""
echo "  3. Build:"
echo "     idf.py build"
echo ""
echo "  4. Flash and monitor:"
echo "     idf.py -p /dev/ttyUSB0 flash monitor"
echo ""
echo "See FIX_SUMMARY.md for detailed troubleshooting."
echo ""
