#!/bin/bash
# validation/gate0-check.sh
# Check if phase can start

PHASE=$1
if [ -z "$PHASE" ]; then
    echo "Usage: gate0-check.sh <phase-number>"
    exit 1
fi

echo "Checking Gate 0 for Phase $PHASE..."

# Check previous phase complete
if [ "$PHASE" -gt 1 ]; then
    PREV=$((PHASE - 1))
    if [ ! -f "progress/phase-$PREV/PHASE_COMPLETE.md" ]; then
        echo "✗ FAIL: Phase $PREV not complete"
        exit 1
    fi
    echo "✓ Phase $PREV complete"
fi

# Check objectives documented
if [ ! -f "progress/phase-$PHASE/OBJECTIVES.md" ]; then
    echo "✗ FAIL: Objectives not documented"
    exit 1
fi
echo "✓ Objectives documented"

# Check for blocking decisions
BLOCKING=$(grep -l "PENDING.*Phase $PHASE" DECISION_LOG.md 2>/dev/null | head -1)
if [ -n "$BLOCKING" ]; then
    echo "⚠ WARNING: Blocking decisions pending for Phase $PHASE"
    grep -B5 "PENDING.*Phase $PHASE" DECISION_LOG.md | head -20
fi

# Check source repos
for repo in src/boxer src/dosbox-staging; do
    if [ ! -d "$repo/.git" ]; then
        echo "✗ FAIL: $repo not a git repo"
        exit 1
    fi
done
echo "✓ Source repositories present"

echo "Gate 0: PASS - Phase $PHASE can start"
