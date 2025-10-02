#!/bin/bash
############################################################
# DSP Warning Analyzer
# Compares OPMODE warnings before and after fix
############################################################

echo "================================================"
echo "DSP48E1 OPMODE Warning Analyzer"
echo "================================================"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

LOG_FILE="hls_run.log"
COSIM_LOG="hls_cnn.prj/sol/sim/verilog/xsim.log"

echo ""
echo "Analyzing HLS run results..."
echo ""

# Check if log exists
if [ ! -f "$LOG_FILE" ]; then
    echo "⚠️  Warning: $LOG_FILE not found"
    echo "   Run: ./test_dsp_fix.sh first"
    echo ""
fi

# Count OPMODE warnings in main log
if [ -f "$LOG_FILE" ]; then
    OPMODE_COUNT=$(grep -c "OPMODE Input Warning" "$LOG_FILE" || echo "0")
    echo "📊 OPMODE Warnings in HLS log: $OPMODE_COUNT"
    
    if [ "$OPMODE_COUNT" -eq 0 ]; then
        echo "   ✅ Excellent! No OPMODE warnings found!"
    elif [ "$OPMODE_COUNT" -lt 10 ]; then
        echo "   ✅ Good! Very few warnings (acceptable)"
    elif [ "$OPMODE_COUNT" -lt 50 ]; then
        echo "   ⚠️  Moderate warnings - consider further optimization"
    else
        echo "   ❌ Many warnings - fix may not be fully effective"
    fi
fi

echo ""

# Check co-simulation log if exists
if [ -f "$COSIM_LOG" ]; then
    COSIM_OPMODE=$(grep -c "OPMODE Input Warning" "$COSIM_LOG" || echo "0")
    echo "📊 OPMODE Warnings in Co-sim: $COSIM_OPMODE"
    
    if [ "$COSIM_OPMODE" -eq 0 ]; then
        echo "   ✅ Perfect! Co-simulation clean!"
    elif [ "$COSIM_OPMODE" -lt 20 ]; then
        echo "   ✅ Acceptable! Minor warnings only"
    else
        echo "   ⚠️  Consider additional optimization"
    fi
else
    echo "📊 Co-simulation log not found (run full flow for complete analysis)"
fi

echo ""

# Check synthesis report
SYNTH_RPT="hls_cnn.prj/sol/syn/report/uut_top_csynth.rpt"
if [ -f "$SYNTH_RPT" ]; then
    echo "================================================"
    echo "Resource Utilization (from synthesis report)"
    echo "================================================"
    
    # Extract resource usage
    if grep -q "== Utilization Estimates" "$SYNTH_RPT"; then
        echo ""
        sed -n '/== Utilization Estimates/,/^$/p' "$SYNTH_RPT" | head -20
        echo ""
        
        # Get DSP count
        DSP_COUNT=$(grep "DSP" "$SYNTH_RPT" | grep -oE '[0-9]+' | head -1 || echo "N/A")
        LUT_COUNT=$(grep "Total LUTs" "$SYNTH_RPT" | grep -oE '[0-9]+' | head -1 || echo "N/A")
        
        echo "Key Metrics:"
        echo "  DSP48E1: $DSP_COUNT (target: 40-60)"
        echo "  LUT: $LUT_COUNT (target: 15k-20k)"
    fi
else
    echo "⚠️  Synthesis report not found"
fi

echo ""
echo "================================================"
echo "Recommendations"
echo "================================================"
echo ""

if [ -f "$LOG_FILE" ]; then
    OPMODE_COUNT=$(grep -c "OPMODE Input Warning" "$LOG_FILE" || echo "0")
    
    if [ "$OPMODE_COUNT" -eq 0 ]; then
        echo "✅ Configuration is optimal!"
        echo ""
        echo "Next steps:"
        echo "  1. Proceed with full synthesis"
        echo "  2. Run implementation"
        echo "  3. Generate bitstream"
    elif [ "$OPMODE_COUNT" -lt 10 ]; then
        echo "✅ Configuration is good!"
        echo ""
        echo "Minor warnings are acceptable. You can:"
        echo "  1. Proceed with current configuration"
        echo "  2. Or try: config_op fmul -impl meddsp (less aggressive)"
    else
        echo "⚠️  Further optimization recommended"
        echo ""
        echo "Try these options in hls_config.tcl:"
        echo "  1. config_op fmul -impl meddsp"
        echo "  2. Increase clock period: set CLKP 15"
        echo "  3. Use: config_op fmul -impl nodsp (last resort)"
    fi
else
    echo "ℹ️  No log file found. Please run:"
    echo "  ./test_dsp_fix.sh"
fi

echo ""
echo "================================================"
echo "For detailed analysis, check:"
echo "  - $LOG_FILE"
echo "  - $SYNTH_RPT"
echo "  - DSP_FIX_SUMMARY.md"
echo "================================================"
echo ""
