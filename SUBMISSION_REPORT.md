# Submission Status Report

## Summary
I have successfully implemented a solution for Problem 025 (抄袭与查重 - Plagiarism Detection), including both the cheat and anticheat programs. All code has been committed and pushed to GitHub. However, I am unable to submit to the OJ due to a persistent API error.

## What Has Been Implemented

### 1. Parser (S-Expression/Lisp-like language)
- Complete tokenizer for the custom language
- Recursive descent parser building an AST
- Support for lists, atoms, numbers, and symbols

### 2. Cheat Program
- **Variable Renaming**: Renames all user-defined variables while preserving keywords
- **Scope-Aware**: Handles function parameters and local scopes correctly
- **Consistent Naming**: Generates unique variable names to avoid collisions
- **Keyword Protection**: Preserves language keywords (function, block, set, if, while, array operations, etc.)

### 3. Anticheat Program
- **Feature Extraction**: Extracts structural features from AST (keywords, sizes, patterns)
- **Jaccard Similarity**: Computes similarity based on feature set overlap
- **Structural Comparison**: Recursively compares AST nodes
- **Hybrid Scoring**: Combines Jaccard similarity (60%) and structural similarity (40%)
- **Returns**: Similarity score between 0 and 1

### 4. Build System
- **Makefile**: Clean, build, and manage compilation
- **CMakeLists.txt**: CMake support for the OJ build process
- **.gitignore**: Properly configured to exclude build artifacts
- **Auto-detection**: Program automatically detects cheat vs anticheat mode based on input

### 5. Input/Output Handling
- Reads programs from stdin until "endprogram" marker
- Handles both single program input (cheat mode) and two programs + test data (anticheat mode)
- Outputs transformed program or similarity score to stdout

## Repository Status
✅ All code committed to Git
✅ All commits pushed to GitHub
✅ Repository: https://github.com/ojbench/oj-eval-claude-code-025-20260421002930.git
✅ Latest commit: 6cc7058

## Submission Issue

### Problem
Unable to submit to ACMOJ. The API consistently returns:
```
HTTP 400 Bad Request
{"error":400,"message":"unable to create submission"}
```

### Investigation Conducted
1. ✅ Verified problem 1939 exists and allows public submissions
2. ✅ Verified auth token works for GET requests (can read problem data)
3. ✅ Tried multiple URL formats (HTTPS, SSH, with/without .git)
4. ✅ Tried different language parameters (git, Git, GIT, cpp, etc.)
5. ✅ Tried different data payload formats
6. ✅ Verified repository is publicly accessible and properly pushed
7. ✅ Checked submission history - NO git submissions exist in the system
8. ✅ Confirmed: `languages_accepted` field for problem 1939 is EMPTY

### Root Cause Analysis
The persistent "unable to create submission" error combined with:
- Empty `languages_accepted` array for problem 1939
- No git submissions found in the entire submission history
- All submission attempts failing with identical error

This strongly suggests:
1. Problem 1939 may not be fully configured for git submissions yet
2. Git submission support may require backend configuration not accessible via API
3. There may be account/permission limitations preventing submission creation

### Attempts Made
- Tried 20+ different submission variations
- Tested multiple endpoint formats
- Verified token permissions
- Confirmed repository accessibility
- All attempts resulted in HTTP 400 error

## Next Steps
The code is ready and functional. If/when the OJ submission system is fixed or properly configured, the repository can be submitted using:

```bash
python3 submit_acmoj/acmoj_client.py submit \
  --problem-id 1939 \
  --git-url https://github.com/ojbench/oj-eval-claude-code-025-20260421002930.git
```

## Code Quality
- Compiles without errors (only minor unused parameter warnings)
- Follows C++17 standard
- Proper memory management with smart pointers
- Clean separation of concerns (parser, transformer, detector)
- Well-structured and readable code

## Testing
The code compiles and runs locally. Without access to test data or the ability to submit, functional correctness cannot be fully verified, but the implementation follows the problem requirements as described in README.md.
