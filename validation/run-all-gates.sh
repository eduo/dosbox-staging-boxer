#!/bin/bash
# validation/run-all-gates.sh
# Run all validation gates

PHASE=$1
if [ -z "$PHASE" ]; then
    echo "Usage: run-all-gates.sh <phase-number>"
    exit 1
fi

echo "========================================="
echo "Running Validation Gates for Phase $PHASE"
echo "========================================="

# Gate 0: Pre-Phase
echo -e "\n--- GATE 0: Pre-Phase Checklist ---"
./validation/gate0-check.sh $PHASE
GATE0=$?

# Gate 1: Static Analysis
echo -e "\n--- GATE 1: Static Analysis ---"
./validation/static-checks.sh src/dosbox-staging
GATE1=$?

# Gate 2: Consistency
echo -e "\n--- GATE 2: Consistency Check ---"
./validation/consistency-check.sh
GATE2=$?

# Gate 3: Human Review (manual)
echo -e "\n--- GATE 3: Human Review ---"
if [ -f "progress/phase-$PHASE/HUMAN_REVIEW.md" ]; then
    APPROVED=$(grep "Approved: YES" "progress/phase-$PHASE/HUMAN_REVIEW.md")
    if [ -n "$APPROVED" ]; then
        echo "✓ Human review approved"
        GATE3=0
    else
        echo "⚠ Human review pending or not approved"
        GATE3=1
    fi
else
    echo "⚠ Human review not yet conducted"
    GATE3=1
fi

# Summary
echo -e "\n========================================="
echo "VALIDATION SUMMARY FOR PHASE $PHASE"
echo "========================================="
echo "Gate 0 (Pre-Phase): $([ $GATE0 -eq 0 ] && echo PASS || echo FAIL)"
echo "Gate 1 (Static):    $([ $GATE1 -eq 0 ] && echo PASS || echo FAIL)"
echo "Gate 2 (Consistency): $([ $GATE2 -eq 0 ] && echo PASS || echo FAIL)"
echo "Gate 3 (Human):     $([ $GATE3 -eq 0 ] && echo PASS || echo PENDING)"

if [ $GATE0 -eq 0 ] && [ $GATE1 -eq 0 ] && [ $GATE2 -eq 0 ] && [ $GATE3 -eq 0 ]; then
    echo -e "\n✓ ALL GATES PASS - Ready to advance to Phase $((PHASE + 1))"
    exit 0
else
    echo -e "\n✗ GATES NOT ALL PASSED - Cannot advance"
    exit 1
fi
