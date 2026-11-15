function showResult(data) {
  document.getElementById("title").textContent =
    data.pageTitle || "Page analysis";

  document.getElementById("score").textContent =
    (data.score ?? "--") + "/100";

  document.getElementById("status").textContent =
    data.status || "No result yet";

  document.getElementById("url").textContent =
    data.pageUrl || "";

  document.getElementById("note").textContent =
    data.note || "Start the C++ backend before scanning.";

  const list = document.getElementById("reasons");
  list.textContent = "";

  (data.reasons || ["No evidence available yet."]).forEach(reason => {
    const li = document.createElement("li");
    li.textContent = reason;
    list.appendChild(li);
  });
}


// Load saved blocking mode.
function loadMode() {
  chrome.storage.local.get("blockingMode", data => {
    const mode = data.blockingMode || "warning";

    document.getElementById("mode").value = mode;
    updateModeInfo(mode);
  });
}


// Show mode description.
function updateModeInfo(mode) {
  const info = document.getElementById("modeInfo");

  if (mode === "strict") {
    info.textContent =
      "High-risk websites will be redirected to the blocked page.";
  } else {
    info.textContent =
      "High-risk websites will show a warning with Continue Once option.";
  }
}


// Save selected mode.
document.getElementById("saveMode").addEventListener("click", () => {
  const mode = document.getElementById("mode").value;

  chrome.storage.local.set({
    blockingMode: mode
  }, () => {
    updateModeInfo(mode);
    alert("Blocking mode saved successfully!");
  });
});


// Update description when selection changes.
document.getElementById("mode").addEventListener("change", event => {
  updateModeInfo(event.target.value);
});


// Refresh analysis result.
function refresh() {
  chrome.storage.local.get("latestResult", data => {
    if (data.latestResult) {
      showResult(data.latestResult);
    }
  });
}

document.getElementById("refresh").addEventListener("click", refresh);

loadMode();
refresh();