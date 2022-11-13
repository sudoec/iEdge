const defaultSettings = {
  extensionIconAction: 'options'
};

// Initializes the extension on first install or update. It loads previous
// settings from Chrome's synced storage if they exist or uses the default
// values otherwise. It also creates the context menu item and shows or hides it
// based on the chosen setting.
chrome.runtime.onInstalled.addListener(() => {
  chrome.storage.sync.get(defaultSettings, items => {
    chrome.storage.sync.set(items);
  });
});

// Opens a tab to the left or right when the extension icon is clicked.
chrome.action.onClicked.addListener(tab => {
  chrome.storage.sync.get('extensionIconAction', items => {
    if (items.extensionIconAction === 'options') {
      chrome.runtime.openOptionsPage();
    }
  });
});

// Opens a tab to the left or right when the respective keyboard shortcut is invoked.
chrome.commands.onCommand.addListener((command) => {
  chrome.tabs.query({active: true, currentWindow: true}, tabs => {
    if (tabs.length && command === 'newTabToTheRight') {
      newAdjacentTab(tabs[0], 'right');
    } else if (tabs.length && command === 'CloseTabToTheLeft') {
      closeAdjacentTab(tabs[0], 'left');
    }
  });
});

/** Opens a new tab to the right or left of the current one. */
function newAdjacentTab(tab, position) {
  const tabIndex = position === 'right' ? tab.index + 1 : tab.index;
  chrome.tabs.create({index: tabIndex, url:"https://google.com"});
  // chrome.tabs.create({index: tabIndex, url:"https://google.com"}, createdTab => {
  //   if (tab.groupId >= 0) {
  //     chrome.tabs.group({groupId: tab.groupId, tabIds: [createdTab.id]});
  //   }
  // });
}

/** Close a tab to the right or left of the current one. */
function closeAdjacentTab(tab, position) {
  if(tab.index == 0) {
    chrome.tabs.remove(tab.id);
    return;
  }
  chrome.tabs.query({currentWindow: true, index: tab.index - 1}, function(tabs) {
      chrome.tabs.update(tabs[0].id, {selected: true});
      chrome.tabs.remove(tab.id);
  });
}

// move a new child tab to the right or left of the parent tab.
chrome.tabs.onCreated.addListener(function(tab) {
  if(tab.hasOwnProperty("openerTabId")) {
    chrome.tabs.get(tab.openerTabId, function(optab) {
      chrome.tabs.move(tab.id, {index: optab.index + 1});  //open on the right
      chrome.tabs.update(tab.id, {selected: true});        //open force foreground
    });
  }
});

/** Process message from SuperDrag class. */
chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
  if (message['flag'] == 'openTable') {
    chrome.tabs.query({active: true, currentWindow: true}, tabs => {
        if (typeof message['url'] == "string") {
          chrome.tabs.create({index: tabs[0].index + 1, url: message['url'], openerTabId: tabs[0].id, active: message['active']});
        } else {
          chrome.tabs.create({ index: tabs[0].index + 1, url: message['url'][i]['url'], openerTabId: tabs[0].id, active: message['active'] });
        }
    });
  }
  sendResponse({status: 'ok'});
});
