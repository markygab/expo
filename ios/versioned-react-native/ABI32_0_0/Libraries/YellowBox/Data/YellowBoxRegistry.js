/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * @flow strict-local
 * @format
 */

'use strict';

const YellowBoxWarning = require('./YellowBoxWarning');

import type {Category} from './YellowBoxCategory';

export type Registry = Map<Category, $ReadOnlyArray<YellowBoxWarning>>;

export type Observer = (registry: Registry) => void;

export type Subscription = $ReadOnly<{|
  unsubscribe: () => void,
|}>;

const observers: Set<{observer: Observer}> = new Set();
const ignorePatterns: Set<string> = new Set();
const registry: Registry = new Map();

let disabled = false;
let projection = new Map();
let updateTimeout = null;

function isWarningIgnored(warning: YellowBoxWarning): boolean {
  for (const pattern of ignorePatterns) {
    if (warning.message.content.includes(pattern)) {
      return true;
    }
  }
  return false;
}

function handleUpdate(): void {
  projection = new Map();
  if (!disabled) {
    for (const [category, warnings] of registry) {
      const filtered = warnings.filter(warning => !isWarningIgnored(warning));
      if (filtered.length > 0) {
        projection.set(category, filtered);
      }
    }
  }
  if (updateTimeout == null) {
    updateTimeout = setImmediate(() => {
      updateTimeout = null;
      for (const {observer} of observers) {
        observer(projection);
      }
    });
  }
}

const YellowBoxRegistry = {
  add({
    args,
    framesToPop,
  }: $ReadOnly<{|
    args: $ReadOnlyArray<mixed>,
    framesToPop: number,
  |}>): void {
    if (typeof args[0] === 'string' && args[0].startsWith('(ADVICE)')) {
      return;
    }
    const {category, message, stack} = YellowBoxWarning.parse({
      args,
      framesToPop: framesToPop + 1,
    });

    let warnings = registry.get(category);
    if (warnings == null) {
      warnings = [];
    }
    warnings = [...warnings, new YellowBoxWarning(message, stack)];

    registry.delete(category);
    registry.set(category, warnings);

    handleUpdate();
  },

  delete(category: Category): void {
    if (registry.has(category)) {
      registry.delete(category);
      handleUpdate();
    }
  },

  clear(): void {
    if (registry.size > 0) {
      registry.clear();
      handleUpdate();
    }
  },

  addIgnorePatterns(patterns: $ReadOnlyArray<string>): void {
    const newPatterns = patterns.filter(
      pattern => !ignorePatterns.has(pattern),
    );
    if (newPatterns.length === 0) {
      return;
    }
    for (const pattern of newPatterns) {
      ignorePatterns.add(pattern);
    }
    handleUpdate();
  },

  setDisabled(value: boolean): void {
    if (value === disabled) {
      return;
    }
    disabled = value;
    handleUpdate();
  },

  isDisabled(): boolean {
    return disabled;
  },

  observe(observer: Observer): Subscription {
    const subscription = {observer};
    observers.add(subscription);
    observer(projection);
    return {
      unsubscribe(): void {
        observers.delete(subscription);
      },
    };
  },
};

module.exports = YellowBoxRegistry;
