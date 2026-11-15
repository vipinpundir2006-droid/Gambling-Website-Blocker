// GAMBLING WEBSITE BLOCKER
// Thin JavaScript controller - C++ handles risk analysis

async function handleAnalysis(message, sender) {
    const evidence = message.evidence || {};
    const pageUrl = evidence.pageUrl || sender.tab?.url || "";

   const fields = [
    "odds", "betActions", "stakeInputs", "marketGroups",
    "depositActions", "casinoEmbeds", "paymentForms",
    "gamblingTerms", "bettingControls", "clickableOdds",
    "walletSignals", "casinoTerms",

    // Mini-game context signals
    "aviatorSignals",
    "vortexSignals",
    "minesSignals",
    "chickenRoadSignals"
];

    const params = new URLSearchParams();

    fields.forEach(key => {
        params.set(key, String(Number(evidence[key]) || 0));
    });

    // Send URL to C++ backend for domain analysis
    params.set("url", pageUrl);

    try {
        // C++ backend performs risk calculation
        const response = await fetch("http://127.0.0.1:8765/analyze", {
            method: "POST",
            headers: {
                "Content-Type": "application/x-www-form-urlencoded;charset=UTF-8"
            },
            body: params.toString()
        });

        if (!response.ok) {
            throw new Error("Backend HTTP " + response.status);
        }

        const result = await response.json();

        const record = {
            ...result,
            pageTitle: evidence.pageTitle || "",
            pageUrl: pageUrl,
            scannedAt: new Date().toLocaleString()
        };

        await chrome.storage.local.set({ latestResult: record });

        const score = Number(result.score) || 0;

        // Extension badge
        const badge = score >= 60 ? "HIGH" :
                      score >= 30 ? "CHECK" : "LOW";

        await chrome.action.setBadgeText({ text: badge });

        await chrome.action.setBadgeBackgroundColor({
            color: score >= 60 ? "#b42318" :
                   score >= 30 ? "#b7791f" : "#287a45"
        });

        const tabId = sender.tab?.id;

        if (tabId === undefined || !pageUrl.startsWith("http")) return;

        const saved = await chrome.storage.local.get([
            "blockingMode", "pendingTabs", "allowedTabs"
        ]);

        const mode = saved.blockingMode || "warning";
        const pendingTabs = saved.pendingTabs || {};
        const allowedTabs = saved.allowedTabs || {};

        if (allowedTabs[tabId] === pageUrl) return;
        if (score < 60) return;
        if (pendingTabs[tabId] === pageUrl) return;

        pendingTabs[tabId] = pageUrl;
        await chrome.storage.local.set({ pendingTabs });

        const warningUrl =
            chrome.runtime.getURL("blocked.html") +
            "?mode=" + encodeURIComponent(mode) +
            "&url=" + encodeURIComponent(pageUrl) +
            "&score=" + encodeURIComponent(score) +
            "&tabId=" + encodeURIComponent(tabId);

        await chrome.tabs.update(tabId, { url: warningUrl });

    } catch (error) {
        await chrome.storage.local.set({
            latestResult: {
                status: "BACKEND OFFLINE",
                score: 0,
                reasons: ["Start the C++ backend and keep its window open."]
            }
        });

        await chrome.action.setBadgeText({ text: "OFF" });
        await chrome.action.setBadgeBackgroundColor({ color: "#666666" });
    }
}

// Messages from other extension files
chrome.runtime.onMessage.addListener((message, sender) => {
    if (message?.type === "ANALYSE_PAGE") {
        handleAnalysis(message, sender);
    }

    if (message?.type === "CONTINUE_ONCE") {
        handleContinueOnce(message.tabId ?? sender.tab?.id);
    }

    if (message?.type === "LEAVE_WEBSITE") {
        handleLeaveWebsite(message.tabId);
    }
});

// Allow website once in Warning Mode
async function handleContinueOnce(tabId) {
    if (tabId === undefined) return;

    const saved = await chrome.storage.local.get([
        "pendingTabs", "allowedTabs"
    ]);

    const pendingTabs = saved.pendingTabs || {};
    const allowedTabs = saved.allowedTabs || {};
    const originalUrl = pendingTabs[tabId];

    if (!originalUrl) return;

    allowedTabs[tabId] = originalUrl;
    delete pendingTabs[tabId];

    await chrome.storage.local.set({
        pendingTabs,
        allowedTabs
    });

    await chrome.tabs.update(tabId, { url: originalUrl });
}

// Leave website
async function handleLeaveWebsite(tabId) {
    if (!Number.isInteger(tabId)) return;

    const saved = await chrome.storage.local.get([
        "pendingTabs", "allowedTabs"
    ]);

    const pendingTabs = saved.pendingTabs || {};
    const allowedTabs = saved.allowedTabs || {};

    delete pendingTabs[tabId];
    delete allowedTabs[tabId];

    await chrome.storage.local.set({
        pendingTabs,
        allowedTabs
    });

    await chrome.tabs.update(tabId, {
        url: "https://www.google.com"
    });
}

// Reset one-time permission when user navigates elsewhere
chrome.tabs.onUpdated.addListener(async (tabId, changeInfo) => {
    if (!changeInfo.url) return;

    const saved = await chrome.storage.local.get("allowedTabs");
    const allowedTabs = saved.allowedTabs || {};

    if (allowedTabs[tabId] && allowedTabs[tabId] !== changeInfo.url) {
        delete allowedTabs[tabId];
        await chrome.storage.local.set({ allowedTabs });
    }
});

// Clean up when tab closes
chrome.tabs.onRemoved.addListener(async (tabId) => {
    const saved = await chrome.storage.local.get([
        "pendingTabs", "allowedTabs"
    ]);

    const pendingTabs = saved.pendingTabs || {};
    const allowedTabs = saved.allowedTabs || {};

    delete pendingTabs[tabId];
    delete allowedTabs[tabId];

    await chrome.storage.local.set({
        pendingTabs,
        allowedTabs
    });
});