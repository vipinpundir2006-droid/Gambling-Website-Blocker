THE GAMBLING WEBSITE BLOCKER v2 - PBL PROTOTYPE
============================================

WHAT THIS PROJECT DOES
- Chrome extension inspects the currently loaded page's DOM structure.
- It looks for combinations of evidence: odds-like values, bet actions,
  stake inputs, repeated market/event containers, deposit actions,
  casino-game embeds and payment forms.
- It sends numeric evidence counts to a C++ backend running on 127.0.0.1.
- The C++ backend calculates a heuristic score and returns reasons.
- The extension displays the result in its popup and toolbar badge.

IMPORTANT LIMITATIONS
- This is a prototype, not a production-grade or legally authoritative classifier.
- It analyses the currently loaded page, not every page on the entire website.
  The extension runs again as you navigate to other pages on the same site.
- It does not click buttons, submit forms, make deposits or place bets.
- It cannot reliably inspect cross-origin iframe internals, closed shadow DOM,
  server-side content not loaded in the browser, or all dynamically rendered data.
- A low score means insufficient evidence was detected, NOT that a website is safe.
- Signals are heuristic and can produce false positives/false negatives.
- It does not automatically block websites yet. It classifies and displays risk.

REQUIREMENTS
- Windows 10/11
- MinGW g++ (or another Windows C++ compiler with Winsock2)
- Google Chrome

BUILD THE C++ BACKEND (MinGW)
1. Open Command Prompt in this project folder.
2. Run:
   g++ backend.cpp -o backend.exe -lws2_32
3. Run:
   backend.exe
4. Keep the backend console window open.

LOAD THE CHROME EXTENSION
1. Open Chrome and visit chrome://extensions
2. Turn on Developer mode.
3. Click Load unpacked.
4. Select the 'extension' folder inside this project.
5. Open a normal website in a new tab. Wait a few seconds.
6. Click the extension icon to see the analysis.

TESTING
- Try a normal informational page and compare its evidence.
- For a controlled demo, use a local HTML test page with clearly labelled mock
  betting-market controls. Do not submit forms or make real payments.
- Results depend on the page content and structure actually loaded.

RISK SCORING (C++ BACKEND)
- Several odds-like values: up to 25 points
- Bet actions: 20 points
- Stake input: 20 points
- Repeated market groups: up to 15 points
- Deposit/wallet action: 10 points
- Casino game embed: 20 points
- Payment form: 5 points
- Strong combinations: bonus points
- 60+ = High risk; 30-59 = Suspicious; below 30 = Low evidence

PBL EXPLANATION
The browser extension performs client-side feature extraction from the DOM.
The C++ backend performs weighted evidence evaluation. The score is based on
multiple page features and combinations, not the domain name. The extension
uses a MutationObserver to re-analyse pages when dynamic page content changes.
