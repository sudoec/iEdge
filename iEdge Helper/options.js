const extensionIconActionElement = document.getElementById(
    'extension-icon-action');
const keyboardShortcutsButtonElement = document.getElementById(
    'keyboard-shortcuts-button');

/** Saves options to Chrome's synced storage. */
function saveOptions() {
  chrome.storage.sync.set({
    extensionIconAction: extensionIconActionElement.value
  });
}

/** Restores options from Chrome's synced storage. */
function restoreOptions() {
  chrome.storage.sync.get(null, items => {
    extensionIconActionElement.value = items.extensionIconAction;
  });
}

// Restores options when the document is fully loaded.
document.addEventListener('DOMContentLoaded', restoreOptions);

// Saves options when a change is detected.
extensionIconActionElement.onchange = saveOptions;

// Opens the keyboard shortcuts configuration page.
keyboardShortcutsButtonElement.onclick = event => {
  chrome.tabs.query({active: true, currentWindow: true}, tabs => {
    chrome.tabs.create({url: 'chrome://extensions/shortcuts',
        index: tabs[0].index + 1});
  });
};
