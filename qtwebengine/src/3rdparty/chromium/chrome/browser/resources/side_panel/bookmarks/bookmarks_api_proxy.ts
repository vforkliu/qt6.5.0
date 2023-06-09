// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {ChromeEvent} from '/tools/typescript/definitions/chrome_event.js';
import {ClickModifiers} from 'chrome://resources/mojo/ui/base/mojom/window_open_disposition.mojom-webui.js';

import {ActionSource, BookmarksPageHandlerFactory, BookmarksPageHandlerRemote} from './bookmarks.mojom-webui.js';

let instance: BookmarksApiProxy|null = null;

export interface BookmarksApiProxy {
  callbackRouter: {[key: string]: ChromeEvent<Function>};
  cutBookmark(id: string): void;
  copyBookmark(id: string): Promise<void>;
  getTopLevelBookmarks(): Promise<chrome.bookmarks.BookmarkTreeNode[]>;
  getFolders(): Promise<chrome.bookmarks.BookmarkTreeNode[]>;
  openBookmark(
      id: string, depth: number, clickModifiers: ClickModifiers,
      source: ActionSource): void;
  pasteToBookmark(parentId: string, destinationId?: string): Promise<void>;
  showContextMenu(id: string, x: number, y: number, source: ActionSource): void;
  showUI(): void;
}

export class BookmarksApiProxyImpl implements BookmarksApiProxy {
  callbackRouter: {[key: string]: ChromeEvent<Function>};
  handler: BookmarksPageHandlerRemote;

  constructor() {
    this.callbackRouter = {
      onChanged: chrome.bookmarks.onChanged,
      onChildrenReordered: chrome.bookmarks.onChildrenReordered,
      onCreated: chrome.bookmarks.onCreated,
      onMoved: chrome.bookmarks.onMoved,
      onRemoved: chrome.bookmarks.onRemoved,
    };

    this.handler = new BookmarksPageHandlerRemote();

    const factory = BookmarksPageHandlerFactory.getRemote();
    factory.createBookmarksPageHandler(
        this.handler.$.bindNewPipeAndPassReceiver());
  }

  cutBookmark(id: string) {
    chrome.bookmarkManagerPrivate.cut([id]);
  }

  copyBookmark(id: string) {
    return new Promise<void>(resolve => {
      chrome.bookmarkManagerPrivate.copy([id], resolve);
    });
  }

  getTopLevelBookmarks() {
    return new Promise<chrome.bookmarks.BookmarkTreeNode[]>(
        resolve => chrome.bookmarks.getTree(results => {
          if (results[0] && results[0].children) {
            let allBookmarks: chrome.bookmarks.BookmarkTreeNode[] = [];
            for (const child of results[0].children) {
              if (child.children) {
                allBookmarks = allBookmarks.concat(child.children);
              }
            }
            resolve(allBookmarks);
            return;
          }
          resolve([]);
        }));
  }

  getFolders() {
    return new Promise<chrome.bookmarks.BookmarkTreeNode[]>(
        resolve => chrome.bookmarks.getTree(results => {
          if (results[0] && results[0].children) {
            resolve(results[0].children);
            return;
          }
          resolve([]);
        }));
  }

  openBookmark(
      id: string, depth: number, clickModifiers: ClickModifiers,
      source: ActionSource) {
    this.handler.openBookmark(BigInt(id), depth, clickModifiers, source);
  }

  pasteToBookmark(parentId: string, destinationId?: string) {
    const destination = destinationId ? [destinationId] : [];
    return new Promise<void>(resolve => {
      chrome.bookmarkManagerPrivate.paste(parentId, destination, resolve);
    });
  }

  showContextMenu(id: string, x: number, y: number, source: ActionSource) {
    this.handler.showContextMenu(id, {x, y}, source);
  }

  showUI() {
    this.handler.showUI();
  }

  static getInstance(): BookmarksApiProxy {
    return instance || (instance = new BookmarksApiProxyImpl());
  }

  static setInstance(obj: BookmarksApiProxy) {
    instance = obj;
  }
}
