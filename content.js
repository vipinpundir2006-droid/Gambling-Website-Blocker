// GAMBLING WEBSITE BLOCKER
// Improved Dynamic DOM Analysis

function countMatches(text, regex) {
    const matches = text.match(regex);
    return matches ? matches.length : 0;
}

function visibleText() {
    return (document.body ? document.body.innerText : "")
        .slice(0, 250000)
        .toLowerCase();
}
function detectMiniGameContext() {

    const games = {
        aviator: /\baviator\b/i,
        vortex: /\bvortex\b/i,
        mines: /\bmines\b/i,
        chickenRoad: /\bchicken\s*road\b/i
    };

    // Gambling-related context
    const strongContext =
        /\b(bet|stake|wager|bet amount|deposit|withdraw|wallet|winnings|cash out|payout|multiplier)\b|₹|inr\b/i;

    const gameSignals = {
        aviator: 0,
        vortex: 0,
        mines: 0,
        chickenRoad: 0
    };

    const elements = document.querySelectorAll(
        'div, section, article, button, a, [role="button"]'
    );

    for (const el of elements) {

        const text = (el.innerText || "")
            .slice(0, 1500)
            .toLowerCase();

        if (!text) continue;

        // Avoid scanning the entire page as one game card
        if (text.length > 1500) continue;

        for (const game in games) {

            if (gameSignals[game] > 0) continue;

            if (!games[game].test(text)) continue;

            let current = el;

            // Check this element and up to 3 parent levels
            for (let i = 0; i < 4 && current; i++) {

                const context = (
                    current.innerText || ""
                ).slice(0, 2000);

                if (strongContext.test(context)) {
                    gameSignals[game] = 1;
                    break;
                }

                current = current.parentElement;
            }
        }
    }

    return gameSignals;
}
function analysePageStructure() {
    const text = visibleText();
    // Detect mini-games
    const miniGames = detectMiniGameContext();
    // Decimal odds
    const oddsPattern =
        /\b(?:1\.[1-9]\d?|2\.[0-9]{1,2}|[3-9]\.[0-9]{1,2}|10\.0)\b/g;

    const odds = countMatches(text, oddsPattern);

    // Betting actions
    const actionPattern =
        /\b(place bet|bet now|add to bet slip|confirm bet|submit bet|back this selection|bet slip|cash out)\b/gi;

    const betActions = countMatches(text, actionPattern);

    // Gambling-related terms
    const gamblingPattern =
        /\b(sportsbook|sports betting|bookmaker|gambling|wager|betting|bet slip|live betting|odds|jackpot|roulette|blackjack|baccarat|poker|slot machine|casino|lottery|win go|trx win go|k3|5d|rummy|fishing|rtp|live casino)\b/gi;

    const gamblingTerms = countMatches(text, gamblingPattern);

    // Casino and gaming terms
    const casinoPattern =
        /\b(casino|roulette|blackjack|baccarat|slots|slot machine|jackpot|live casino|lottery|win go|trx win go|k3|5d|rummy|fishing)\b/gi;

    const casinoTerms = countMatches(text, casinoPattern);

    // Betting controls and clickable odds
    let bettingControls = 0;
    let clickableOdds = 0;

    const clickableElements = document.querySelectorAll(
        'button, a, [role="button"], [role="option"], [role="checkbox"], [onclick]'
    );

    for (const el of clickableElements) {
        const label = [
            el.innerText,
            el.getAttribute("aria-label"),
            el.getAttribute("title"),
            el.getAttribute("data-testid")
        ].filter(Boolean).join(" ").toLowerCase();

        if (/place bet|bet now|add to bet slip|confirm bet|submit bet|back this selection|select odds|bet slip/.test(label)) {
            bettingControls++;
        }

        if (/\b(?:1\.[1-9]\d?|2\.[0-9]{1,2}|[3-9]\.[0-9]{1,2}|10\.0)\b/.test(label)) {
            clickableOdds++;
        }
    }

    // Stake inputs
    let stakeInputs = 0;

    const inputs = document.querySelectorAll(
        'input, [role="spinbutton"]'
    );

    for (const el of inputs) {
        const hint = [
            el.getAttribute("placeholder"),
            el.getAttribute("aria-label"),
            el.getAttribute("name"),
            el.getAttribute("id")
        ].filter(Boolean).join(" ").toLowerCase();

        if (/stake|wager|bet amount|amount to bet/.test(hint)) {
            stakeInputs++;
        }
    }

    // Market groups
    const marketNodes = document.querySelectorAll(
        '[class*="market"], [data-testid*="market"], [class*="event-card"], [data-testid*="event"]'
    );

    const marketGroups = Math.min(marketNodes.length, 100);

    // Deposit and withdrawal
    const depositPattern =
        /\b(deposit funds|make a deposit|add funds|top up wallet|withdraw winnings|deposit|withdraw)\b/gi;

    const depositActions = countMatches(text, depositPattern);

    // Wallet signals
    const walletPattern =
        /\b(wallet balance|my wallet|add money|available balance|account balance|deposit|withdraw)\b/gi;

    const walletSignals = countMatches(text, walletPattern);

    // Casino embeds
    let casinoEmbeds = 0;

    const frames = document.querySelectorAll(
        'iframe, object, embed'
    );

    for (const frame of frames) {
        const context = [
            frame.getAttribute("title"),
            frame.getAttribute("aria-label"),
            frame.getAttribute("data-game"),
            frame.getAttribute("class")
        ].filter(Boolean).join(" ").toLowerCase();

        if (/slot|roulette|blackjack|casino|live.?game|game.?provider/.test(context)) {
            casinoEmbeds++;
        }
    }

    // Payment forms
    let paymentForms = 0;

    for (const form of document.querySelectorAll("form")) {
        const context = [
            form.innerText,
            form.getAttribute("action"),
            form.getAttribute("aria-label")
        ].filter(Boolean).join(" ").toLowerCase();

        if (/card number|payment method|upi|wallet|deposit|add funds/.test(context)) {
            paymentForms++;
        }
    }

    return {
        odds: Math.min(odds, 100),
        betActions: Math.min(betActions, 50),
        stakeInputs: Math.min(stakeInputs, 50),
        marketGroups,
        depositActions: Math.min(depositActions, 50),
        casinoEmbeds: Math.min(casinoEmbeds, 20),
        paymentForms: Math.min(paymentForms, 20),
        gamblingTerms: Math.min(gamblingTerms, 100),
        bettingControls: Math.min(bettingControls, 50),
        clickableOdds: Math.min(clickableOdds, 100),
        walletSignals: Math.min(walletSignals, 50),
        casinoTerms: Math.min(casinoTerms, 100),
        aviatorSignals: miniGames.aviator,
        vortexSignals: miniGames.vortex,
        minesSignals: miniGames.mines,
        chickenRoadSignals: miniGames.chickenRoad,
        pageTitle: document.title || "Untitled page",
        pageUrl: location.href
    };
}

// Send scan to background controller
let scanTimer = null;

function requestScan() {
    clearTimeout(scanTimer);

    scanTimer = setTimeout(() => {
        try {
            const evidence = analysePageStructure();

            console.log("GWB EVIDENCE:", evidence);

            chrome.runtime.sendMessage({
                type: "ANALYSE_PAGE",
                evidence: evidence
            }, () => {
                try {
                    if (chrome.runtime.lastError) {
                        console.debug(
                            "GWB message:",
                            chrome.runtime.lastError.message
                        );
                    }
                } catch (e) {
                    // Old extension context
                }
            });

        } catch (error) {
            if (error.message &&
                !error.message.includes("Extension context invalidated")) {
                console.error("GWB Error:", error);
            }
        }
    }, 1200);
}

// Initial scan
requestScan();

// Observe dynamically added content
const observer = new MutationObserver(() => {
    requestScan();
});

if (document.documentElement) {
    observer.observe(document.documentElement, {
        childList: true,
        subtree: true,
        characterData: true
    });
}

// Handle SPA route changes
window.addEventListener("hashchange", requestScan);
window.addEventListener("popstate", requestScan);
window.addEventListener("pageshow", requestScan);