# Agent Task Report Templates

Use these templates for consistent task reporting across all phases.

---

## TASK COMPLETION REPORT

**Save to**: `progress/phase-X/tasks/TASK-X-Y.md`

```markdown
# TASK [X-Y]: [Task Title]

## Metadata
- **Agent ID**: [Descriptive ID, e.g., "CMake-Builder", "Hook-Designer"]
- **Phase**: [1-8]
- **Date Started**: [ISO timestamp]
- **Date Completed**: [ISO timestamp]
- **Status**: COMPLETE / BLOCKED / PARTIAL

---

## Objective
[Single sentence describing what this task accomplished]

---

## Work Completed

### Summary
[2-3 sentence summary of what was done]

### Files Created
| File | Lines | Purpose |
|------|-------|---------|
| `path/to/file.ext` | +[N] | [Brief description] |

### Files Modified
| File | Changes | Lines Added/Removed |
|------|---------|---------------------|
| `path/to/file.ext` | [Description of changes] | +[N] / -[M] |

### Files Deleted
- [List any files removed, if applicable]

---

## Technical Details

### Approach Taken
[Describe the implementation approach and why it was chosen]

### Key Implementation Points
1. [Important technical detail]
2. [Important technical detail]
3. [Important technical detail]

### Code Samples
[Include small, relevant code snippets if helpful]

```cpp
// Example of key code added
#ifdef BOXER_INTEGRATED
// Implementation here
#endif
```

---

## Decisions Made

### Within-Scope Decisions (No Escalation Required)
1. **[Decision]**: [Choice made]
   - Justification: [Why this choice]
   - Alternatives considered: [What else was possible]

### Deferred to Human (Escalation Required)
1. **[Decision Name]**
   - Issue: [What needs deciding]
   - Options:
     - A: [Option] - Pros: [List], Cons: [List]
     - B: [Option] - Pros: [List], Cons: [List]
   - My recommendation: [Option X] because [reasoning]
   - Blocking: [YES/NO - Does this block further work?]

---

## Validation Results

### Gate 1: Static Analysis
```bash
$ ./validation/static-checks.sh src/dosbox-staging
[Output here]
```
**Result**: PASS / FAIL

### Additional Validations
```bash
$ [Command run]
[Output]
```
**Result**: PASS / FAIL

### Issues Found and Fixed
- [Issue]: [How it was fixed]

---

## Concerns Identified

### Technical Concerns
1. **[Concern]**: [Details]
   - Impact: [What could go wrong]
   - Mitigation: [How to address]
   - Severity: LOW / MEDIUM / HIGH

### Process Concerns
1. **[Concern]**: [Details]

### Performance Concerns
1. **[Concern]**: [Details]

---

## Assumptions Made

### Verified Assumptions
- [Assumption]: [How it was verified]

### Unverified Assumptions
- [Assumption]: [Why it couldn't be verified, risk if wrong]

---

## Testing Performed

### What Was Tested
1. [Test]: [Result]
2. [Test]: [Result]

### What Could Not Be Tested (Yet)
1. [Test]: [Why not possible] - [When it can be tested]

---

## Documentation Updates

### Files Updated
- `DECISION_LOG.md`: [What was added]
- `PROGRESS.md`: [What was updated]
- [Other documentation]

### Documentation Created
- [New doc]: [Purpose]

---

## Next Steps

### Immediate Next Task
**TASK [X-Z]**: [Task name]
- Ready to start: [YES/NO]
- Dependencies: [What's needed]
- Estimated hours: [From plan]

### Blocked Tasks
- **TASK [X-W]**: Blocked by [Decision/Issue]

### Recommendations
1. [Specific actionable recommendation]
2. [Specific actionable recommendation]

---

## Time Tracking

- **Estimated hours**: [From plan]
- **Actual hours**: [What was spent]
- **Variance**: [Over/Under by X hours]
- **Reason for variance**: [If significant]

---

## References Used

### Analysis Documents
- `analysis/[file].md` lines [X-Y]: [What was referenced]

### Source Code
- `src/[file]`: [What was examined]

### External Resources
- [If any]

---

## Sign-off

Task completed by: [Agent ID]
Date: [ISO timestamp]
Ready for orchestrator review: YES / NO

### Orchestrator Notes (filled by orchestrator)
- Reviewed: [Date]
- Approved: YES / NO
- Follow-up required: [Details]
```

---

## PHASE COMPLETION REPORT

**Save to**: `progress/phase-X/PHASE_COMPLETE.md`

```markdown
# Phase [X] Completion Report: [Phase Name]

## Phase Overview
- **Start Date**: [ISO date]
- **End Date**: [ISO date]
- **Duration**: [Weeks/Days]
- **Status**: COMPLETE / BLOCKED / PARTIAL

---

## Objectives Met

### Primary Objectives
- [x] [Objective 1] - Completed
- [x] [Objective 2] - Completed
- [ ] [Objective 3] - Not completed (reason)

### Success Criteria Check
| Criterion | Target | Actual | Pass/Fail |
|-----------|--------|--------|-----------|
| [Criterion] | [Target] | [Actual] | ✅/❌ |

---

## Deliverables

### Code Deliverables
- [File/Component]: [Status and location]

### Documentation Deliverables
- [Document]: [Location]

### Validation Deliverables
- [Test/Script]: [Status]

---

## Tasks Completed

| Task ID | Name | Hours Est. | Hours Actual | Status |
|---------|------|------------|--------------|--------|
| [X-1] | [Name] | [Est] | [Actual] | ✅ |
| [X-2] | [Name] | [Est] | [Actual] | ✅ |

**Total Phase Hours**: [Actual] / [Estimated] ([X]% of estimate)

---

## Decisions Made

### Human Decisions Required
1. **DEC-XXX**: [Title]
   - Status: RESOLVED / PENDING
   - Decision: [What was decided]

### Agent Decisions
1. [Decision]: [What was chosen and why]

---

## Validation Gates

### Gate 1: Static Analysis
- Status: PASS / FAIL
- Details: [Summary]

### Gate 2: Consistency Check
- Status: PASS / FAIL
- Details: [Summary]

### Gate 3: Human Review
- Status: APPROVED / PENDING
- Reviewer: [Name]
- Date: [ISO date]
- Comments: [Notes]

---

## Issues and Resolutions

### Blockers Encountered
1. [Blocker]: [How it was resolved]

### Technical Issues
1. [Issue]: [Resolution]

### Process Issues
1. [Issue]: [Resolution]

---

## Lessons Learned

### What Worked Well
1. [Practice/Approach]
2. [Practice/Approach]

### What Could Be Improved
1. [Issue]: [Suggested improvement]
2. [Issue]: [Suggested improvement]

### Recommendations for Future Phases
1. [Recommendation]
2. [Recommendation]

---

## Risk Assessment Update

### New Risks Identified
1. [Risk]: [Impact], [Mitigation]

### Risks Mitigated
1. [Risk]: [How it was addressed]

### Ongoing Risks
1. [Risk]: [Current status]

---

## Next Phase Preparation

### Phase [X+1] Prerequisites
- [ ] [Prerequisite 1]
- [ ] [Prerequisite 2]

### Pending Decisions for Phase [X+1]
1. DEC-XXX: [Must be resolved before Phase X+1]

### Recommended Adjustments
- [Any changes to upcoming phase plan]

---

## Metrics

### Code Metrics
- Lines of code added: [N]
- Files created: [N]
- Files modified: [N]
- #ifdef BOXER_INTEGRATED blocks: [N]

### Quality Metrics
- Static analysis issues found: [N]
- Issues fixed: [N]
- TODOs remaining: [N]

### Process Metrics
- Tasks completed on first attempt: [N/Total]
- Escalations to human: [N]
- Average task completion time: [Hours]

---

## Final Sign-off

### Orchestrator Approval
- Phase objectives met: YES / NO
- Ready for next phase: YES / NO
- Signed: [Agent/Orchestrator ID]
- Date: [ISO timestamp]

### Human Approval
- Phase deliverables acceptable: YES / NO
- Decisions appropriately escalated: YES / NO
- Ready to proceed: YES / NO
- Signed: [Human name]
- Date: [ISO timestamp]

---

## Attachments

- [Link to all task reports]
- [Link to validation outputs]
- [Link to code changes]
```

---

## DECISION ESCALATION TEMPLATE

**Use when agent needs human decision**

```markdown
# Decision Escalation: [Short Title]

## Context
- **Phase**: [Current phase]
- **Task**: [Current task]
- **Agent**: [Agent ID]
- **Date**: [ISO timestamp]

---

## Issue

[Clear description of what decision is needed]

### Why This Decision Matters
[Impact on project if wrong choice made]

### Why I Can't Decide This
[What makes this beyond agent scope]

---

## Options

### Option A: [Name]
**Description**: [What this entails]

**Implementation**:
```cpp
// How code would look
```

**Pros**:
- [Benefit 1]
- [Benefit 2]

**Cons**:
- [Drawback 1]
- [Drawback 2]

**Estimated Impact**: [Hours added/saved]

---

### Option B: [Name]
**Description**: [What this entails]

**Implementation**:
```cpp
// How code would look
```

**Pros**:
- [Benefit 1]
- [Benefit 2]

**Cons**:
- [Drawback 1]
- [Drawback 2]

**Estimated Impact**: [Hours added/saved]

---

### Option C: [Name]
[Same format as above]

---

## Analysis Recommendation
[What the original analysis documents suggested, if applicable]

## Agent Recommendation

**I recommend Option [X]** because:
1. [Reason 1]
2. [Reason 2]
3. [Reason 3]

**Confidence Level**: HIGH / MEDIUM / LOW

---

## Blocking Information

### Is This Blocking?
[YES/NO] - [What cannot proceed without this decision]

### Can Proceed Partially?
[YES/NO] - [What work can continue]

### Time Sensitivity
[Urgent / Can wait / Flexible]

---

## Questions for Human

1. [Specific question about the decision]
2. [Specific question about preferences]
3. [Any context needed from human]

---

## Human Response

**Decision**: [To be filled by human]
**Rationale**: [To be filled by human]
**Date**: [To be filled by human]

---

## Post-Decision Action

After human decides:
1. Update DECISION_LOG.md with resolution
2. Proceed with implementation
3. Document any adjustments to plan
```

---

## BLOCKER REPORT TEMPLATE

**Use when work cannot proceed**

```markdown
# Blocker Report

## Identification
- **Date Identified**: [ISO timestamp]
- **Agent**: [Agent ID]
- **Phase**: [Current phase]
- **Task**: [Affected task]

---

## Blocker Description

[Clear description of what is preventing progress]

### Type
- [ ] Technical (code won't compile, missing dependency)
- [ ] Information (need clarification, missing docs)
- [ ] Decision (need human input)
- [ ] Resource (missing access, tools)
- [ ] External (waiting on upstream, third party)

---

## Impact

### What Cannot Proceed
- Task [X-Y]: [Why blocked]
- Task [X-Z]: [Why blocked]

### Estimated Delay
[Hours/Days if not resolved]

### Cascade Effects
[What else will be delayed]

---

## Attempted Resolutions

1. **Attempt**: [What was tried]
   - Result: [What happened]
   - Why insufficient: [Explanation]

2. **Attempt**: [What was tried]
   - Result: [What happened]
   - Why insufficient: [Explanation]

---

## Proposed Resolution Path

### Option 1 (Preferred)
[How to unblock]
- Requires: [What's needed]
- Who can provide: [Human/External/Agent]
- Timeline: [How long]

### Option 2 (Alternative)
[Alternative approach]
- Requires: [What's needed]
- Trade-offs: [What's compromised]

### Option 3 (Workaround)
[Temporary solution]
- Limitations: [What won't work]
- Technical debt: [What needs fixing later]

---

## Assistance Needed

- [ ] Human decision required
- [ ] Additional information needed
- [ ] External resource access required
- [ ] Technical expertise consultation
- [ ] Scope clarification

**Specific ask**: [Exactly what is needed from human]

---

## Resolution

### Status
- [ ] Open
- [ ] In Progress
- [ ] Resolved
- [ ] Deferred

### Resolution Details
[To be filled when resolved]

### Lessons Learned
[To be filled when resolved]

**Resolved Date**: [ISO timestamp]
**Resolved By**: [Who resolved it]
```

---

These templates ensure:
1. Consistent reporting across all agents
2. Complete information capture
3. Easy tracking of progress
4. Clear escalation paths
5. Audit trail for decisions
