const params = new URLSearchParams(location.search);

const mode = params.get("mode") || "warning";
const originalUrl = params.get("url") || "";
const score = params.get("score") || "0";
const tabId = Number(params.get("tabId"));

document.getElementById("score").textContent = score + "/100";

document.getElementById("website").textContent =
    "Website: " + originalUrl;

const continueButton = document.getElementById("continue");

if (mode === "strict") {
    document.getElementById("heading").textContent =
        "Website Blocked!";

    document.getElementById("message").textContent =
        "Strict Mode is enabled. Access to this high-risk page is blocked.";

    continueButton.style.display = "none";

} else {
    document.getElementById("heading").textContent =
        "High Risk Warning!";

    document.getElementById("message").textContent =
        "This page contains multiple gambling-related signals.";

    continueButton.addEventListener("click", () => {
        chrome.runtime.sendMessage({
            type: "CONTINUE_ONCE",
            tabId: tabId
        });
    });
}

document.getElementById("leave").addEventListener("click", async () => {

    if (!Number.isInteger(tabId)) {
        alert("Tab ID not found!");
        return;
    }

    await chrome.storage.local.get("pendingTabs", async (data) => {

        const pendingTabs = data.pendingTabs || {};

        delete pendingTabs[tabId];

        await chrome.storage.local.set({
            pendingTabs: pendingTabs
        });

        chrome.tabs.update(tabId, {
            url: "https://www.google.com"
        });

    });
});