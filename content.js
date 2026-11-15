// GAMBLING WEBSITE BLOCKER V2
// JavaScript only collects browser DOM evidence. C++ performs pattern matching
// and risk scoring in the local backend.

function compactText(value, limit) {
    return String(value || "")
        .replace(/[^\x20-\x7E]/g, " ")
        .replace(/\s+/g, " ")
        .trim()
        .slice(0, limit);
}

function collectValues(selector, getValue, maxItems, itemLimit) {
    return Array.from(document.querySelectorAll(selector))
        .slice(0, maxItems)
        .map(element => compactText(getValue(element), itemLimit))
        .filter(Boolean)
        .join(" | ");
}

function collectPageEvidence() {
    const pageText = compactText(
        document.body ? document.body.innerText : "",
        7000
    );

    const clickableLabels = collectValues(
        'button, a, [role="button"], [role="option"], [role="checkbox"], [onclick]',
        element => [
            element.innerText,
            element.getAttribute("aria-label"),
            element.getAttribute("title"),
            element.getAttribute("data-testid")
        ].filter(Boolean).join(" "),
        80,
        100
    ).slice(0, 2500);

    const inputHints = collectValues(
        'input, [role="spinbutton"]',
        element => [
            element.getAttribute("placeholder"),
            element.getAttribute("aria-label"),
            element.getAttribute("name"),
            element.getAttribute("id")
        ].filter(Boolean).join(" "),
        60,
        100
    ).slice(0, 1800);

    const marketHints = Array.from(document.querySelectorAll(
        '[class*="market"], [data-testid*="market"], [class*="event-card"], [data-testid*="event"]'
    )).slice(0, 100).map(() => "market").join(" ");

    const frameHints = collectValues(
        'iframe, object, embed',
        element => [
            element.getAttribute("title"),
            element.getAttribute("aria-label"),
            element.getAttribute("data-game"),
            element.getAttribute("class")
        ].filter(Boolean).join(" "),
        20,
        100
    );

    const formHints = collectValues(
        "form",
        element => [
            element.innerText,
            element.getAttribute("action"),
            element.getAttribute("aria-label")
        ].filter(Boolean).join(" "),
        20,
        150
    );

    return {
        pageText,
        clickableLabels,
        inputHints,
        marketHints,
        frameHints,
        formHints,
        pageTitle: compactText(document.title, 160) || "Untitled page",
        pageUrl: location.href
    };
}

let scanTimer = null;

function requestScan() {
    clearTimeout(scanTimer);

    scanTimer = setTimeout(() => {
        try {
            chrome.runtime.sendMessage({
                type: "ANALYSE_PAGE",
                evidence: collectPageEvidence()
            }, () => {
                if (chrome.runtime.lastError) {
                    console.debug("GWB message:", chrome.runtime.lastError.message);
                }
            });
        } catch (error) {
            if (error.message &&
                !error.message.includes("Extension context invalidated")) {
                console.error("GWB scan error:", error);
            }
        }
    }, 1200);
}

requestScan();

// Trusted developer websites
const hostname = location.hostname.toLowerCase();

const isTrustedDeveloperSite =
    hostname === "github.com" ||
    hostname.endsWith(".github.com");

if (!isTrustedDeveloperSite) {
    requestScan();

    const observer = new MutationObserver(requestScan);

    if (document.documentElement) {
        observer.observe(document.documentElement, {
            childList: true,
            subtree: true,
            characterData: true
        });
    }

    window.addEventListener("hashchange", requestScan);
    window.addEventListener("popstate", requestScan);
    window.addEventListener("pageshow", requestScan);
} else {
    console.log("GWB: GitHub trusted - scanning skipped.");
}