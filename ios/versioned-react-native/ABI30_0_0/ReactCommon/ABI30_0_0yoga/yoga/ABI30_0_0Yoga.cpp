/**
 * Copyright (c) 2014-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ABI30_0_0Yoga.h"
#include <string.h>
#include <algorithm>
#include "ABI30_0_0Utils.h"
#include "ABI30_0_0YGNode.h"
#include "ABI30_0_0YGNodePrint.h"
#include "ABI30_0_0Yoga-internal.h"

#ifdef _MSC_VER
#include <float.h>

/* define fmaxf if < VC12 */
#if _MSC_VER < 1800
__forceinline const float fmaxf(const float a, const float b) {
  if (!ABI30_0_0YGFloatIsUndefined(a) && !ABI30_0_0YGFloatIsUndefined(b)) {
    return (a > b) ? a : b;
  }
  return ABI30_0_0YGFloatIsUndefined(a) ? b : a;
}
#endif
#endif

#ifdef ANDROID
static int ABI30_0_0YGAndroidLog(const ABI30_0_0YGConfigRef config,
                        const ABI30_0_0YGNodeRef node,
                        ABI30_0_0YGLogLevel level,
                        const char *format,
                        va_list args);
#else
static int ABI30_0_0YGDefaultLog(const ABI30_0_0YGConfigRef config,
                        const ABI30_0_0YGNodeRef node,
                        ABI30_0_0YGLogLevel level,
                        const char *format,
                        va_list args);
#endif

static ABI30_0_0YGConfig gABI30_0_0YGConfigDefaults = {
    .experimentalFeatures =
        {
            [ABI30_0_0YGExperimentalFeatureWebFlexBasis] = false,
        },
    .useWebDefaults = false,
    .useLegacyStretchBehaviour = false,
    .shouldDiffLayoutWithoutLegacyStretchBehaviour = false,
    .pointScaleFactor = 1.0f,
#ifdef ANDROID
    .logger = &ABI30_0_0YGAndroidLog,
#else
    .logger = &ABI30_0_0YGDefaultLog,
#endif
    .cloneNodeCallback = nullptr,
    .context = nullptr,
};

const ABI30_0_0YGValue ABI30_0_0YGValueZero = {.value = 0, .unit = ABI30_0_0YGUnitPoint};
const ABI30_0_0YGValue ABI30_0_0YGValueUndefined = {ABI30_0_0YGUndefined, ABI30_0_0YGUnitUndefined};
const ABI30_0_0YGValue ABI30_0_0YGValueAuto = {ABI30_0_0YGUndefined, ABI30_0_0YGUnitAuto};

#ifdef ANDROID
#include <android/log.h>
static int ABI30_0_0YGAndroidLog(const ABI30_0_0YGConfigRef config,
                        const ABI30_0_0YGNodeRef node,
                        ABI30_0_0YGLogLevel level,
                        const char *format,
                        va_list args) {
  int androidLevel = ABI30_0_0YGLogLevelDebug;
  switch (level) {
    case ABI30_0_0YGLogLevelFatal:
      androidLevel = ANDROID_LOG_FATAL;
      break;
    case ABI30_0_0YGLogLevelError:
      androidLevel = ANDROID_LOG_ERROR;
      break;
    case ABI30_0_0YGLogLevelWarn:
      androidLevel = ANDROID_LOG_WARN;
      break;
    case ABI30_0_0YGLogLevelInfo:
      androidLevel = ANDROID_LOG_INFO;
      break;
    case ABI30_0_0YGLogLevelDebug:
      androidLevel = ANDROID_LOG_DEBUG;
      break;
    case ABI30_0_0YGLogLevelVerbose:
      androidLevel = ANDROID_LOG_VERBOSE;
      break;
  }
  const int result = __android_log_vprint(androidLevel, "yoga", format, args);
  return result;
}
#else
#define ABI30_0_0YG_UNUSED(x) (void)(x);

static int ABI30_0_0YGDefaultLog(const ABI30_0_0YGConfigRef config,
                        const ABI30_0_0YGNodeRef node,
                        ABI30_0_0YGLogLevel level,
                        const char *format,
                        va_list args) {
  ABI30_0_0YG_UNUSED(config);
  ABI30_0_0YG_UNUSED(node);
  switch (level) {
    case ABI30_0_0YGLogLevelError:
    case ABI30_0_0YGLogLevelFatal:
      return vfprintf(stderr, format, args);
    case ABI30_0_0YGLogLevelWarn:
    case ABI30_0_0YGLogLevelInfo:
    case ABI30_0_0YGLogLevelDebug:
    case ABI30_0_0YGLogLevelVerbose:
    default:
      return vprintf(format, args);
  }
}

#undef ABI30_0_0YG_UNUSED
#endif

bool ABI30_0_0YGFloatIsUndefined(const float value) {
  // Value of a float in the case of it being not defined is 10.1E20. Earlier
  // it used to be NAN, the benefit of which was that if NAN is involved in any
  // mathematical expression the result was NAN. But since we want to have
  // `-ffast-math` flag being used by compiler which assumes that the floating
  // point values are not NAN and Inf, we represent ABI30_0_0YGUndefined as 10.1E20. But
  // now if ABI30_0_0YGUndefined is involved in any mathematical operations this
  // value(10.1E20) would change. So the following check makes sure that if the
  // value is outside a range (-10E8, 10E8) then it is undefined.
  return value >= 10E8 || value <= -10E8;
}

const ABI30_0_0YGValue* ABI30_0_0YGComputedEdgeValue(
    const std::array<ABI30_0_0YGValue, ABI30_0_0YGEdgeCount>& edges,
    const ABI30_0_0YGEdge edge,
    const ABI30_0_0YGValue* const defaultValue) {
  if (edges[edge].unit != ABI30_0_0YGUnitUndefined) {
    return &edges[edge];
  }

  if ((edge == ABI30_0_0YGEdgeTop || edge == ABI30_0_0YGEdgeBottom) &&
      edges[ABI30_0_0YGEdgeVertical].unit != ABI30_0_0YGUnitUndefined) {
    return &edges[ABI30_0_0YGEdgeVertical];
  }

  if ((edge == ABI30_0_0YGEdgeLeft || edge == ABI30_0_0YGEdgeRight || edge == ABI30_0_0YGEdgeStart || edge == ABI30_0_0YGEdgeEnd) &&
      edges[ABI30_0_0YGEdgeHorizontal].unit != ABI30_0_0YGUnitUndefined) {
    return &edges[ABI30_0_0YGEdgeHorizontal];
  }

  if (edges[ABI30_0_0YGEdgeAll].unit != ABI30_0_0YGUnitUndefined) {
    return &edges[ABI30_0_0YGEdgeAll];
  }

  if (edge == ABI30_0_0YGEdgeStart || edge == ABI30_0_0YGEdgeEnd) {
    return &ABI30_0_0YGValueUndefined;
  }

  return defaultValue;
}

void* ABI30_0_0YGNodeGetContext(ABI30_0_0YGNodeRef node) {
  return node->getContext();
}

void ABI30_0_0YGNodeSetContext(ABI30_0_0YGNodeRef node, void* context) {
  return node->setContext(context);
}

ABI30_0_0YGMeasureFunc ABI30_0_0YGNodeGetMeasureFunc(ABI30_0_0YGNodeRef node) {
  return node->getMeasure();
}

void ABI30_0_0YGNodeSetMeasureFunc(ABI30_0_0YGNodeRef node, ABI30_0_0YGMeasureFunc measureFunc) {
  node->setMeasureFunc(measureFunc);
}

ABI30_0_0YGBaselineFunc ABI30_0_0YGNodeGetBaselineFunc(ABI30_0_0YGNodeRef node) {
  return node->getBaseline();
}

void ABI30_0_0YGNodeSetBaselineFunc(ABI30_0_0YGNodeRef node, ABI30_0_0YGBaselineFunc baselineFunc) {
  node->setBaseLineFunc(baselineFunc);
}

ABI30_0_0YGDirtiedFunc ABI30_0_0YGNodeGetDirtiedFunc(ABI30_0_0YGNodeRef node) {
  return node->getDirtied();
}

void ABI30_0_0YGNodeSetDirtiedFunc(ABI30_0_0YGNodeRef node, ABI30_0_0YGDirtiedFunc dirtiedFunc) {
  node->setDirtiedFunc(dirtiedFunc);
}

ABI30_0_0YGPrintFunc ABI30_0_0YGNodeGetPrintFunc(ABI30_0_0YGNodeRef node) {
  return node->getPrintFunc();
}

void ABI30_0_0YGNodeSetPrintFunc(ABI30_0_0YGNodeRef node, ABI30_0_0YGPrintFunc printFunc) {
  node->setPrintFunc(printFunc);
}

bool ABI30_0_0YGNodeGetHasNewLayout(ABI30_0_0YGNodeRef node) {
  return node->getHasNewLayout();
}

void ABI30_0_0YGNodeSetHasNewLayout(ABI30_0_0YGNodeRef node, bool hasNewLayout) {
  node->setHasNewLayout(hasNewLayout);
}

ABI30_0_0YGNodeType ABI30_0_0YGNodeGetNodeType(ABI30_0_0YGNodeRef node) {
  return node->getNodeType();
}

void ABI30_0_0YGNodeSetNodeType(ABI30_0_0YGNodeRef node, ABI30_0_0YGNodeType nodeType) {
  return node->setNodeType(nodeType);
}

bool ABI30_0_0YGNodeIsDirty(ABI30_0_0YGNodeRef node) {
  return node->isDirty();
}

bool ABI30_0_0YGNodeLayoutGetDidUseLegacyFlag(const ABI30_0_0YGNodeRef node) {
  return node->didUseLegacyFlag();
}

void ABI30_0_0YGNodeMarkDirtyAndPropogateToDescendants(const ABI30_0_0YGNodeRef node) {
  return node->markDirtyAndPropogateDownwards();
}

int32_t gNodeInstanceCount = 0;
int32_t gConfigInstanceCount = 0;

WIN_EXPORT ABI30_0_0YGNodeRef ABI30_0_0YGNodeNewWithConfig(const ABI30_0_0YGConfigRef config) {
  const ABI30_0_0YGNodeRef node = new ABI30_0_0YGNode();
  ABI30_0_0YGAssertWithConfig(
      config, node != nullptr, "Could not allocate memory for node");
  gNodeInstanceCount++;

  if (config->useWebDefaults) {
    node->setStyleFlexDirection(ABI30_0_0YGFlexDirectionRow);
    node->setStyleAlignContent(ABI30_0_0YGAlignStretch);
  }
  node->setConfig(config);
  return node;
}

ABI30_0_0YGNodeRef ABI30_0_0YGNodeNew(void) {
  return ABI30_0_0YGNodeNewWithConfig(&gABI30_0_0YGConfigDefaults);
}

ABI30_0_0YGNodeRef ABI30_0_0YGNodeClone(ABI30_0_0YGNodeRef oldNode) {
  ABI30_0_0YGNodeRef node = new ABI30_0_0YGNode(*oldNode);
  ABI30_0_0YGAssertWithConfig(
      oldNode->getConfig(),
      node != nullptr,
      "Could not allocate memory for node");
  gNodeInstanceCount++;
  node->setParent(nullptr);
  return node;
}

static ABI30_0_0YGConfigRef ABI30_0_0YGConfigClone(const ABI30_0_0YGConfig& oldConfig) {
  const ABI30_0_0YGConfigRef config = new ABI30_0_0YGConfig(oldConfig);
  ABI30_0_0YGAssert(config != nullptr, "Could not allocate memory for config");
  if (config == nullptr) {
    abort();
  }
  gConfigInstanceCount++;
  return config;
}

static ABI30_0_0YGNodeRef ABI30_0_0YGNodeDeepClone(ABI30_0_0YGNodeRef oldNode) {
  ABI30_0_0YGNodeRef node = ABI30_0_0YGNodeClone(oldNode);
  ABI30_0_0YGVector vec = ABI30_0_0YGVector();
  vec.reserve(oldNode->getChildren().size());
  ABI30_0_0YGNodeRef childNode = nullptr;
  for (auto& item : oldNode->getChildren()) {
    childNode = ABI30_0_0YGNodeDeepClone(item);
    childNode->setParent(node);
    vec.push_back(childNode);
  }
  node->setChildren(vec);

  if (oldNode->getConfig() != nullptr) {
    node->setConfig(ABI30_0_0YGConfigClone(*(oldNode->getConfig())));
  }

  if (oldNode->getNextChild() != nullptr) {
    node->setNextChild(ABI30_0_0YGNodeDeepClone(oldNode->getNextChild()));
  }

  return node;
}

void ABI30_0_0YGNodeFree(const ABI30_0_0YGNodeRef node) {
  if (node->getParent()) {
    node->getParent()->removeChild(node);
    node->setParent(nullptr);
  }

  const uint32_t childCount = ABI30_0_0YGNodeGetChildCount(node);
  for (uint32_t i = 0; i < childCount; i++) {
    const ABI30_0_0YGNodeRef child = ABI30_0_0YGNodeGetChild(node, i);
    child->setParent(nullptr);
  }

  node->clearChildren();
  delete node;
  gNodeInstanceCount--;
}

static void ABI30_0_0YGConfigFreeRecursive(const ABI30_0_0YGNodeRef root) {
  if (root->getConfig() != nullptr) {
    gConfigInstanceCount--;
    delete root->getConfig();
  }
  // Delete configs recursively for childrens
  for (uint32_t i = 0; i < root->getChildrenCount(); ++i) {
    ABI30_0_0YGConfigFreeRecursive(root->getChild(i));
  }
}

void ABI30_0_0YGNodeFreeRecursive(const ABI30_0_0YGNodeRef root) {
  while (ABI30_0_0YGNodeGetChildCount(root) > 0) {
    const ABI30_0_0YGNodeRef child = ABI30_0_0YGNodeGetChild(root, 0);
    if (child->getParent() != root) {
      // Don't free shared nodes that we don't own.
      break;
    }
    ABI30_0_0YGNodeRemoveChild(root, child);
    ABI30_0_0YGNodeFreeRecursive(child);
  }
  ABI30_0_0YGNodeFree(root);
}

void ABI30_0_0YGNodeReset(const ABI30_0_0YGNodeRef node) {
  ABI30_0_0YGAssertWithNode(node,
                   ABI30_0_0YGNodeGetChildCount(node) == 0,
                   "Cannot reset a node which still has children attached");
  ABI30_0_0YGAssertWithNode(
      node,
      node->getParent() == nullptr,
      "Cannot reset a node still attached to a parent");

  node->clearChildren();

  const ABI30_0_0YGConfigRef config = node->getConfig();
  *node = ABI30_0_0YGNode();
  if (config->useWebDefaults) {
    node->setStyleFlexDirection(ABI30_0_0YGFlexDirectionRow);
    node->setStyleAlignContent(ABI30_0_0YGAlignStretch);
  }
  node->setConfig(config);
}

int32_t ABI30_0_0YGNodeGetInstanceCount(void) {
  return gNodeInstanceCount;
}

int32_t ABI30_0_0YGConfigGetInstanceCount(void) {
  return gConfigInstanceCount;
}

// Export only for C#
ABI30_0_0YGConfigRef ABI30_0_0YGConfigGetDefault() {
  return &gABI30_0_0YGConfigDefaults;
}

ABI30_0_0YGConfigRef ABI30_0_0YGConfigNew(void) {
  const ABI30_0_0YGConfigRef config = (const ABI30_0_0YGConfigRef)malloc(sizeof(ABI30_0_0YGConfig));
  ABI30_0_0YGAssert(config != nullptr, "Could not allocate memory for config");
  if (config == nullptr) {
    abort();
  }
  gConfigInstanceCount++;
  memcpy(config, &gABI30_0_0YGConfigDefaults, sizeof(ABI30_0_0YGConfig));
  return config;
}

void ABI30_0_0YGConfigFree(const ABI30_0_0YGConfigRef config) {
  free(config);
  gConfigInstanceCount--;
}

void ABI30_0_0YGConfigCopy(const ABI30_0_0YGConfigRef dest, const ABI30_0_0YGConfigRef src) {
  memcpy(dest, src, sizeof(ABI30_0_0YGConfig));
}

void ABI30_0_0YGNodeInsertChild(const ABI30_0_0YGNodeRef node, const ABI30_0_0YGNodeRef child, const uint32_t index) {
  ABI30_0_0YGAssertWithNode(
      node,
      child->getParent() == nullptr,
      "Child already has a parent, it must be removed first.");
  ABI30_0_0YGAssertWithNode(
      node,
      node->getMeasure() == nullptr,
      "Cannot add child: Nodes with measure functions cannot have children.");

  node->cloneChildrenIfNeeded();
  node->insertChild(child, index);
  child->setParent(node);
  node->markDirtyAndPropogate();
}

void ABI30_0_0YGNodeRemoveChild(const ABI30_0_0YGNodeRef parent, const ABI30_0_0YGNodeRef excludedChild) {
  // This algorithm is a forked variant from cloneChildrenIfNeeded in ABI30_0_0YGNode
  // that excludes a child.
  const uint32_t childCount = ABI30_0_0YGNodeGetChildCount(parent);

  if (childCount == 0) {
    // This is an empty set. Nothing to remove.
    return;
  }
  const ABI30_0_0YGNodeRef firstChild = ABI30_0_0YGNodeGetChild(parent, 0);
  if (firstChild->getParent() == parent) {
    // If the first child has this node as its parent, we assume that it is already unique.
    // We can now try to delete a child in this list.
    if (parent->removeChild(excludedChild)) {
      excludedChild->setLayout(
          ABI30_0_0YGNode().getLayout()); // layout is no longer valid
      excludedChild->setParent(nullptr);
      parent->markDirtyAndPropogate();
    }
    return;
  }
  // Otherwise we have to clone the node list except for the child we're trying to delete.
  // We don't want to simply clone all children, because then the host will need to free
  // the clone of the child that was just deleted.
  const ABI30_0_0YGNodeClonedFunc cloneNodeCallback =
      parent->getConfig()->cloneNodeCallback;
  uint32_t nextInsertIndex = 0;
  for (uint32_t i = 0; i < childCount; i++) {
    const ABI30_0_0YGNodeRef oldChild = parent->getChild(i);
    if (excludedChild == oldChild) {
      // Ignore the deleted child. Don't reset its layout or parent since it is still valid
      // in the other parent. However, since this parent has now changed, we need to mark it
      // as dirty.
      parent->markDirtyAndPropogate();
      continue;
    }
    const ABI30_0_0YGNodeRef newChild = ABI30_0_0YGNodeClone(oldChild);
    parent->replaceChild(newChild, nextInsertIndex);
    newChild->setParent(parent);
    if (cloneNodeCallback) {
      cloneNodeCallback(oldChild, newChild, parent, nextInsertIndex);
    }
    nextInsertIndex++;
  }
  while (nextInsertIndex < childCount) {
    parent->removeChild(nextInsertIndex);
    nextInsertIndex++;
  }
}

void ABI30_0_0YGNodeRemoveAllChildren(const ABI30_0_0YGNodeRef parent) {
  const uint32_t childCount = ABI30_0_0YGNodeGetChildCount(parent);
  if (childCount == 0) {
    // This is an empty set already. Nothing to do.
    return;
  }
  const ABI30_0_0YGNodeRef firstChild = ABI30_0_0YGNodeGetChild(parent, 0);
  if (firstChild->getParent() == parent) {
    // If the first child has this node as its parent, we assume that this child set is unique.
    for (uint32_t i = 0; i < childCount; i++) {
      const ABI30_0_0YGNodeRef oldChild = ABI30_0_0YGNodeGetChild(parent, i);
      oldChild->setLayout(ABI30_0_0YGNode().getLayout()); // layout is no longer valid
      oldChild->setParent(nullptr);
    }
    parent->clearChildren();
    parent->markDirtyAndPropogate();
    return;
  }
  // Otherwise, we are not the owner of the child set. We don't have to do anything to clear it.
  parent->setChildren(ABI30_0_0YGVector());
  parent->markDirtyAndPropogate();
}

ABI30_0_0YGNodeRef ABI30_0_0YGNodeGetChild(const ABI30_0_0YGNodeRef node, const uint32_t index) {
  if (index < node->getChildren().size()) {
    return node->getChild(index);
  }
  return nullptr;
}

uint32_t ABI30_0_0YGNodeGetChildCount(const ABI30_0_0YGNodeRef node) {
  return static_cast<uint32_t>(node->getChildren().size());
}

ABI30_0_0YGNodeRef ABI30_0_0YGNodeGetParent(const ABI30_0_0YGNodeRef node) {
  return node->getParent();
}

void ABI30_0_0YGNodeMarkDirty(const ABI30_0_0YGNodeRef node) {
  ABI30_0_0YGAssertWithNode(
      node,
      node->getMeasure() != nullptr,
      "Only leaf nodes with custom measure functions"
      "should manually mark themselves as dirty");

  node->markDirtyAndPropogate();
}

void ABI30_0_0YGNodeCopyStyle(const ABI30_0_0YGNodeRef dstNode, const ABI30_0_0YGNodeRef srcNode) {
  if (!(dstNode->getStyle() == srcNode->getStyle())) {
    dstNode->setStyle(srcNode->getStyle());
    dstNode->markDirtyAndPropogate();
  }
}

float ABI30_0_0YGNodeStyleGetFlexGrow(const ABI30_0_0YGNodeRef node) {
  return ABI30_0_0YGFloatIsUndefined(node->getStyle().flexGrow)
      ? kDefaultFlexGrow
      : node->getStyle().flexGrow;
}

float ABI30_0_0YGNodeStyleGetFlexShrink(const ABI30_0_0YGNodeRef node) {
  return ABI30_0_0YGFloatIsUndefined(node->getStyle().flexShrink)
      ? (node->getConfig()->useWebDefaults ? kWebDefaultFlexShrink
                                           : kDefaultFlexShrink)
      : node->getStyle().flexShrink;
}

#define ABI30_0_0YG_NODE_STYLE_PROPERTY_SETTER_IMPL(                               \
    type, name, paramName, instanceName)                                  \
  void ABI30_0_0YGNodeStyleSet##name(const ABI30_0_0YGNodeRef node, const type paramName) { \
    if (node->getStyle().instanceName != paramName) {                     \
      ABI30_0_0YGStyle style = node->getStyle();                                   \
      style.instanceName = paramName;                                     \
      node->setStyle(style);                                              \
      node->markDirtyAndPropogate();                                      \
    }                                                                     \
  }

#define ABI30_0_0YG_NODE_STYLE_PROPERTY_SETTER_UNIT_IMPL(                               \
    type, name, paramName, instanceName)                                       \
  void ABI30_0_0YGNodeStyleSet##name(const ABI30_0_0YGNodeRef node, const type paramName) {      \
    ABI30_0_0YGValue value = {                                                          \
        .value = paramName,                                                    \
        .unit = ABI30_0_0YGFloatIsUndefined(paramName) ? ABI30_0_0YGUnitUndefined : ABI30_0_0YGUnitPoint, \
    };                                                                         \
    if ((node->getStyle().instanceName.value != value.value &&                 \
         value.unit != ABI30_0_0YGUnitUndefined) ||                                     \
        node->getStyle().instanceName.unit != value.unit) {                    \
      ABI30_0_0YGStyle style = node->getStyle();                                        \
      style.instanceName = value;                                              \
      node->setStyle(style);                                                   \
      node->markDirtyAndPropogate();                                           \
    }                                                                          \
  }                                                                            \
                                                                               \
  void ABI30_0_0YGNodeStyleSet##name##Percent(                                          \
      const ABI30_0_0YGNodeRef node, const type paramName) {                            \
    ABI30_0_0YGValue value = {                                                          \
        .value = paramName,                                                    \
        .unit =                                                                \
            ABI30_0_0YGFloatIsUndefined(paramName) ? ABI30_0_0YGUnitUndefined : ABI30_0_0YGUnitPercent,   \
    };                                                                         \
    if ((node->getStyle().instanceName.value != value.value &&                 \
         value.unit != ABI30_0_0YGUnitUndefined) ||                                     \
        node->getStyle().instanceName.unit != value.unit) {                    \
      ABI30_0_0YGStyle style = node->getStyle();                                        \
                                                                               \
      style.instanceName = value;                                              \
      node->setStyle(style);                                                   \
      node->markDirtyAndPropogate();                                           \
    }                                                                          \
  }

#define ABI30_0_0YG_NODE_STYLE_PROPERTY_SETTER_UNIT_AUTO_IMPL(                          \
    type, name, paramName, instanceName)                                       \
  void ABI30_0_0YGNodeStyleSet##name(const ABI30_0_0YGNodeRef node, const type paramName) {      \
    ABI30_0_0YGValue value = {                                                          \
        .value = paramName,                                                    \
        .unit = ABI30_0_0YGFloatIsUndefined(paramName) ? ABI30_0_0YGUnitUndefined : ABI30_0_0YGUnitPoint, \
    };                                                                         \
    if ((node->getStyle().instanceName.value != value.value &&                 \
         value.unit != ABI30_0_0YGUnitUndefined) ||                                     \
        node->getStyle().instanceName.unit != value.unit) {                    \
      ABI30_0_0YGStyle style = node->getStyle();                                        \
      style.instanceName = value;                                              \
      node->setStyle(style);                                                   \
      node->markDirtyAndPropogate();                                           \
    }                                                                          \
  }                                                                            \
                                                                               \
  void ABI30_0_0YGNodeStyleSet##name##Percent(                                          \
      const ABI30_0_0YGNodeRef node, const type paramName) {                            \
    if (node->getStyle().instanceName.value != paramName ||                    \
        node->getStyle().instanceName.unit != ABI30_0_0YGUnitPercent) {                 \
      ABI30_0_0YGStyle style = node->getStyle();                                        \
      style.instanceName.value = paramName;                                    \
      style.instanceName.unit =                                                \
          ABI30_0_0YGFloatIsUndefined(paramName) ? ABI30_0_0YGUnitAuto : ABI30_0_0YGUnitPercent;          \
      node->setStyle(style);                                                   \
      node->markDirtyAndPropogate();                                           \
    }                                                                          \
  }                                                                            \
                                                                               \
  void ABI30_0_0YGNodeStyleSet##name##Auto(const ABI30_0_0YGNodeRef node) {                      \
    if (node->getStyle().instanceName.unit != ABI30_0_0YGUnitAuto) {                    \
      ABI30_0_0YGStyle style = node->getStyle();                                        \
      style.instanceName.value = ABI30_0_0YGUndefined;                                  \
      style.instanceName.unit = ABI30_0_0YGUnitAuto;                                    \
      node->setStyle(style);                                                   \
      node->markDirtyAndPropogate();                                           \
    }                                                                          \
  }

#define ABI30_0_0YG_NODE_STYLE_PROPERTY_IMPL(type, name, paramName, instanceName)  \
  ABI30_0_0YG_NODE_STYLE_PROPERTY_SETTER_IMPL(type, name, paramName, instanceName) \
                                                                          \
  type ABI30_0_0YGNodeStyleGet##name(const ABI30_0_0YGNodeRef node) {                       \
    return node->getStyle().instanceName;                                 \
  }

#define ABI30_0_0YG_NODE_STYLE_PROPERTY_UNIT_IMPL(type, name, paramName, instanceName) \
  ABI30_0_0YG_NODE_STYLE_PROPERTY_SETTER_UNIT_IMPL(                                    \
      float, name, paramName, instanceName)                                   \
                                                                              \
  type ABI30_0_0YGNodeStyleGet##name(const ABI30_0_0YGNodeRef node) {                           \
    return node->getStyle().instanceName;                                     \
  }

#define ABI30_0_0YG_NODE_STYLE_PROPERTY_UNIT_AUTO_IMPL(      \
    type, name, paramName, instanceName)            \
  ABI30_0_0YG_NODE_STYLE_PROPERTY_SETTER_UNIT_AUTO_IMPL(     \
      float, name, paramName, instanceName)         \
                                                    \
  type ABI30_0_0YGNodeStyleGet##name(const ABI30_0_0YGNodeRef node) { \
    return node->getStyle().instanceName;           \
  }

#define ABI30_0_0YG_NODE_STYLE_EDGE_PROPERTY_UNIT_AUTO_IMPL(type, name, instanceName) \
  void ABI30_0_0YGNodeStyleSet##name##Auto(const ABI30_0_0YGNodeRef node, const ABI30_0_0YGEdge edge) { \
    if (node->getStyle().instanceName[edge].unit != ABI30_0_0YGUnitAuto) {            \
      ABI30_0_0YGStyle style = node->getStyle();                                      \
      style.instanceName[edge].value = ABI30_0_0YGUndefined;                          \
      style.instanceName[edge].unit = ABI30_0_0YGUnitAuto;                            \
      node->setStyle(style);                                                 \
      node->markDirtyAndPropogate();                                         \
    }                                                                        \
  }

#define ABI30_0_0YG_NODE_STYLE_EDGE_PROPERTY_UNIT_IMPL(                                 \
    type, name, paramName, instanceName)                                       \
  void ABI30_0_0YGNodeStyleSet##name(                                                   \
      const ABI30_0_0YGNodeRef node, const ABI30_0_0YGEdge edge, const float paramName) {        \
    ABI30_0_0YGValue value = {                                                          \
        .value = paramName,                                                    \
        .unit = ABI30_0_0YGFloatIsUndefined(paramName) ? ABI30_0_0YGUnitUndefined : ABI30_0_0YGUnitPoint, \
    };                                                                         \
    if ((node->getStyle().instanceName[edge].value != value.value &&           \
         value.unit != ABI30_0_0YGUnitUndefined) ||                                     \
        node->getStyle().instanceName[edge].unit != value.unit) {              \
      ABI30_0_0YGStyle style = node->getStyle();                                        \
      style.instanceName[edge] = value;                                        \
      node->setStyle(style);                                                   \
      node->markDirtyAndPropogate();                                           \
    }                                                                          \
  }                                                                            \
                                                                               \
  void ABI30_0_0YGNodeStyleSet##name##Percent(                                          \
      const ABI30_0_0YGNodeRef node, const ABI30_0_0YGEdge edge, const float paramName) {        \
    ABI30_0_0YGValue value = {                                                          \
        .value = paramName,                                                    \
        .unit =                                                                \
            ABI30_0_0YGFloatIsUndefined(paramName) ? ABI30_0_0YGUnitUndefined : ABI30_0_0YGUnitPercent,   \
    };                                                                         \
    if ((node->getStyle().instanceName[edge].value != value.value &&           \
         value.unit != ABI30_0_0YGUnitUndefined) ||                                     \
        node->getStyle().instanceName[edge].unit != value.unit) {              \
      ABI30_0_0YGStyle style = node->getStyle();                                        \
      style.instanceName[edge] = value;                                        \
      node->setStyle(style);                                                   \
      node->markDirtyAndPropogate();                                           \
    }                                                                          \
  }                                                                            \
                                                                               \
  WIN_STRUCT(type)                                                             \
  ABI30_0_0YGNodeStyleGet##name(const ABI30_0_0YGNodeRef node, const ABI30_0_0YGEdge edge) {              \
    return WIN_STRUCT_REF(node->getStyle().instanceName[edge]);                \
  }

#define ABI30_0_0YG_NODE_STYLE_EDGE_PROPERTY_IMPL(type, name, paramName, instanceName)  \
  void ABI30_0_0YGNodeStyleSet##name(                                                   \
      const ABI30_0_0YGNodeRef node, const ABI30_0_0YGEdge edge, const float paramName) {        \
    ABI30_0_0YGValue value = {                                                          \
        .value = paramName,                                                    \
        .unit = ABI30_0_0YGFloatIsUndefined(paramName) ? ABI30_0_0YGUnitUndefined : ABI30_0_0YGUnitPoint, \
    };                                                                         \
    if ((node->getStyle().instanceName[edge].value != value.value &&           \
         value.unit != ABI30_0_0YGUnitUndefined) ||                                     \
        node->getStyle().instanceName[edge].unit != value.unit) {              \
      ABI30_0_0YGStyle style = node->getStyle();                                        \
      style.instanceName[edge] = value;                                        \
      node->setStyle(style);                                                   \
      node->markDirtyAndPropogate();                                           \
    }                                                                          \
  }                                                                            \
                                                                               \
  float ABI30_0_0YGNodeStyleGet##name(const ABI30_0_0YGNodeRef node, const ABI30_0_0YGEdge edge) {        \
    return node->getStyle().instanceName[edge].value;                          \
  }

#define ABI30_0_0YG_NODE_LAYOUT_PROPERTY_IMPL(type, name, instanceName) \
  type ABI30_0_0YGNodeLayoutGet##name(const ABI30_0_0YGNodeRef node) {           \
    return node->getLayout().instanceName;                     \
  }

#define ABI30_0_0YG_NODE_LAYOUT_RESOLVED_PROPERTY_IMPL(type, name, instanceName) \
  type ABI30_0_0YGNodeLayoutGet##name(const ABI30_0_0YGNodeRef node, const ABI30_0_0YGEdge edge) { \
    ABI30_0_0YGAssertWithNode(                                                   \
        node,                                                           \
        edge <= ABI30_0_0YGEdgeEnd,                                              \
        "Cannot get layout properties of multi-edge shorthands");       \
                                                                        \
    if (edge == ABI30_0_0YGEdgeLeft) {                                           \
      if (node->getLayout().direction == ABI30_0_0YGDirectionRTL) {              \
        return node->getLayout().instanceName[ABI30_0_0YGEdgeEnd];               \
      } else {                                                          \
        return node->getLayout().instanceName[ABI30_0_0YGEdgeStart];             \
      }                                                                 \
    }                                                                   \
                                                                        \
    if (edge == ABI30_0_0YGEdgeRight) {                                          \
      if (node->getLayout().direction == ABI30_0_0YGDirectionRTL) {              \
        return node->getLayout().instanceName[ABI30_0_0YGEdgeStart];             \
      } else {                                                          \
        return node->getLayout().instanceName[ABI30_0_0YGEdgeEnd];               \
      }                                                                 \
    }                                                                   \
                                                                        \
    return node->getLayout().instanceName[edge];                        \
  }

// ABI30_0_0YG_NODE_PROPERTY_IMPL(void *, Context, context, context);
// ABI30_0_0YG_NODE_PROPERTY_IMPL(ABI30_0_0YGPrintFunc, PrintFunc, printFunc, print);
// ABI30_0_0YG_NODE_PROPERTY_IMPL(bool, HasNewLayout, hasNewLayout, hasNewLayout);
// ABI30_0_0YG_NODE_PROPERTY_IMPL(ABI30_0_0YGNodeType, NodeType, nodeType, nodeType);

ABI30_0_0YG_NODE_STYLE_PROPERTY_IMPL(ABI30_0_0YGDirection, Direction, direction, direction);
ABI30_0_0YG_NODE_STYLE_PROPERTY_IMPL(ABI30_0_0YGFlexDirection, FlexDirection, flexDirection, flexDirection);
ABI30_0_0YG_NODE_STYLE_PROPERTY_IMPL(ABI30_0_0YGJustify, JustifyContent, justifyContent, justifyContent);
ABI30_0_0YG_NODE_STYLE_PROPERTY_IMPL(ABI30_0_0YGAlign, AlignContent, alignContent, alignContent);
ABI30_0_0YG_NODE_STYLE_PROPERTY_IMPL(ABI30_0_0YGAlign, AlignItems, alignItems, alignItems);
ABI30_0_0YG_NODE_STYLE_PROPERTY_IMPL(ABI30_0_0YGAlign, AlignSelf, alignSelf, alignSelf);
ABI30_0_0YG_NODE_STYLE_PROPERTY_IMPL(ABI30_0_0YGPositionType, PositionType, positionType, positionType);
ABI30_0_0YG_NODE_STYLE_PROPERTY_IMPL(ABI30_0_0YGWrap, FlexWrap, flexWrap, flexWrap);
ABI30_0_0YG_NODE_STYLE_PROPERTY_IMPL(ABI30_0_0YGOverflow, Overflow, overflow, overflow);
ABI30_0_0YG_NODE_STYLE_PROPERTY_IMPL(ABI30_0_0YGDisplay, Display, display, display);

ABI30_0_0YG_NODE_STYLE_PROPERTY_IMPL(float, Flex, flex, flex);
ABI30_0_0YG_NODE_STYLE_PROPERTY_SETTER_IMPL(float, FlexGrow, flexGrow, flexGrow);
ABI30_0_0YG_NODE_STYLE_PROPERTY_SETTER_IMPL(float, FlexShrink, flexShrink, flexShrink);
ABI30_0_0YG_NODE_STYLE_PROPERTY_UNIT_AUTO_IMPL(ABI30_0_0YGValue, FlexBasis, flexBasis, flexBasis);

ABI30_0_0YG_NODE_STYLE_EDGE_PROPERTY_UNIT_IMPL(ABI30_0_0YGValue, Position, position, position);
ABI30_0_0YG_NODE_STYLE_EDGE_PROPERTY_UNIT_IMPL(ABI30_0_0YGValue, Margin, margin, margin);
ABI30_0_0YG_NODE_STYLE_EDGE_PROPERTY_UNIT_AUTO_IMPL(ABI30_0_0YGValue, Margin, margin);
ABI30_0_0YG_NODE_STYLE_EDGE_PROPERTY_UNIT_IMPL(ABI30_0_0YGValue, Padding, padding, padding);
ABI30_0_0YG_NODE_STYLE_EDGE_PROPERTY_IMPL(float, Border, border, border);

ABI30_0_0YG_NODE_STYLE_PROPERTY_UNIT_AUTO_IMPL(ABI30_0_0YGValue, Width, width, dimensions[ABI30_0_0YGDimensionWidth]);
ABI30_0_0YG_NODE_STYLE_PROPERTY_UNIT_AUTO_IMPL(ABI30_0_0YGValue, Height, height, dimensions[ABI30_0_0YGDimensionHeight]);
ABI30_0_0YG_NODE_STYLE_PROPERTY_UNIT_IMPL(ABI30_0_0YGValue, MinWidth, minWidth, minDimensions[ABI30_0_0YGDimensionWidth]);
ABI30_0_0YG_NODE_STYLE_PROPERTY_UNIT_IMPL(ABI30_0_0YGValue, MinHeight, minHeight, minDimensions[ABI30_0_0YGDimensionHeight]);
ABI30_0_0YG_NODE_STYLE_PROPERTY_UNIT_IMPL(ABI30_0_0YGValue, MaxWidth, maxWidth, maxDimensions[ABI30_0_0YGDimensionWidth]);
ABI30_0_0YG_NODE_STYLE_PROPERTY_UNIT_IMPL(ABI30_0_0YGValue, MaxHeight, maxHeight, maxDimensions[ABI30_0_0YGDimensionHeight]);

// Yoga specific properties, not compatible with flexbox specification
ABI30_0_0YG_NODE_STYLE_PROPERTY_IMPL(float, AspectRatio, aspectRatio, aspectRatio);

ABI30_0_0YG_NODE_LAYOUT_PROPERTY_IMPL(float, Left, position[ABI30_0_0YGEdgeLeft]);
ABI30_0_0YG_NODE_LAYOUT_PROPERTY_IMPL(float, Top, position[ABI30_0_0YGEdgeTop]);
ABI30_0_0YG_NODE_LAYOUT_PROPERTY_IMPL(float, Right, position[ABI30_0_0YGEdgeRight]);
ABI30_0_0YG_NODE_LAYOUT_PROPERTY_IMPL(float, Bottom, position[ABI30_0_0YGEdgeBottom]);
ABI30_0_0YG_NODE_LAYOUT_PROPERTY_IMPL(float, Width, dimensions[ABI30_0_0YGDimensionWidth]);
ABI30_0_0YG_NODE_LAYOUT_PROPERTY_IMPL(float, Height, dimensions[ABI30_0_0YGDimensionHeight]);
ABI30_0_0YG_NODE_LAYOUT_PROPERTY_IMPL(ABI30_0_0YGDirection, Direction, direction);
ABI30_0_0YG_NODE_LAYOUT_PROPERTY_IMPL(bool, HadOverflow, hadOverflow);

ABI30_0_0YG_NODE_LAYOUT_RESOLVED_PROPERTY_IMPL(float, Margin, margin);
ABI30_0_0YG_NODE_LAYOUT_RESOLVED_PROPERTY_IMPL(float, Border, border);
ABI30_0_0YG_NODE_LAYOUT_RESOLVED_PROPERTY_IMPL(float, Padding, padding);

uint32_t gCurrentGenerationCount = 0;

bool ABI30_0_0YGLayoutNodeInternal(const ABI30_0_0YGNodeRef node,
                          const float availableWidth,
                          const float availableHeight,
                          const ABI30_0_0YGDirection parentDirection,
                          const ABI30_0_0YGMeasureMode widthMeasureMode,
                          const ABI30_0_0YGMeasureMode heightMeasureMode,
                          const float parentWidth,
                          const float parentHeight,
                          const bool performLayout,
                          const char *reason,
                          const ABI30_0_0YGConfigRef config);

static void ABI30_0_0YGNodePrintInternal(const ABI30_0_0YGNodeRef node,
                                const ABI30_0_0YGPrintOptions options) {
  std::string str;
  facebook::yoga::ABI30_0_0YGNodeToString(&str, node, options, 0);
  ABI30_0_0YGLog(node, ABI30_0_0YGLogLevelDebug, str.c_str());
}

void ABI30_0_0YGNodePrint(const ABI30_0_0YGNodeRef node, const ABI30_0_0YGPrintOptions options) {
  ABI30_0_0YGNodePrintInternal(node, options);
}

const std::array<ABI30_0_0YGEdge, 4> leading = {
    {ABI30_0_0YGEdgeTop, ABI30_0_0YGEdgeBottom, ABI30_0_0YGEdgeLeft, ABI30_0_0YGEdgeRight}};

const std::array<ABI30_0_0YGEdge, 4> trailing = {
    {ABI30_0_0YGEdgeBottom, ABI30_0_0YGEdgeTop, ABI30_0_0YGEdgeRight, ABI30_0_0YGEdgeLeft}};
static const std::array<ABI30_0_0YGEdge, 4> pos = {{
    ABI30_0_0YGEdgeTop,
    ABI30_0_0YGEdgeBottom,
    ABI30_0_0YGEdgeLeft,
    ABI30_0_0YGEdgeRight,
}};

static const std::array<ABI30_0_0YGDimension, 4> dim = {
    {ABI30_0_0YGDimensionHeight, ABI30_0_0YGDimensionHeight, ABI30_0_0YGDimensionWidth, ABI30_0_0YGDimensionWidth}};

static inline float ABI30_0_0YGNodePaddingAndBorderForAxis(const ABI30_0_0YGNodeRef node,
                                                  const ABI30_0_0YGFlexDirection axis,
                                                  const float widthSize) {
  return node->getLeadingPaddingAndBorder(axis, widthSize) +
      node->getTrailingPaddingAndBorder(axis, widthSize);
}

static inline ABI30_0_0YGAlign ABI30_0_0YGNodeAlignItem(const ABI30_0_0YGNodeRef node, const ABI30_0_0YGNodeRef child) {
  const ABI30_0_0YGAlign align = child->getStyle().alignSelf == ABI30_0_0YGAlignAuto
      ? node->getStyle().alignItems
      : child->getStyle().alignSelf;
  if (align == ABI30_0_0YGAlignBaseline &&
      ABI30_0_0YGFlexDirectionIsColumn(node->getStyle().flexDirection)) {
    return ABI30_0_0YGAlignFlexStart;
  }
  return align;
}

static float ABI30_0_0YGBaseline(const ABI30_0_0YGNodeRef node) {
  if (node->getBaseline() != nullptr) {
    const float baseline = node->getBaseline()(
        node,
        node->getLayout().measuredDimensions[ABI30_0_0YGDimensionWidth],
        node->getLayout().measuredDimensions[ABI30_0_0YGDimensionHeight]);
    ABI30_0_0YGAssertWithNode(node,
                     !ABI30_0_0YGFloatIsUndefined(baseline),
                     "Expect custom baseline function to not return NaN");
    return baseline;
  }

  ABI30_0_0YGNodeRef baselineChild = nullptr;
  const uint32_t childCount = ABI30_0_0YGNodeGetChildCount(node);
  for (uint32_t i = 0; i < childCount; i++) {
    const ABI30_0_0YGNodeRef child = ABI30_0_0YGNodeGetChild(node, i);
    if (child->getLineIndex() > 0) {
      break;
    }
    if (child->getStyle().positionType == ABI30_0_0YGPositionTypeAbsolute) {
      continue;
    }
    if (ABI30_0_0YGNodeAlignItem(node, child) == ABI30_0_0YGAlignBaseline) {
      baselineChild = child;
      break;
    }

    if (baselineChild == nullptr) {
      baselineChild = child;
    }
  }

  if (baselineChild == nullptr) {
    return node->getLayout().measuredDimensions[ABI30_0_0YGDimensionHeight];
  }

  const float baseline = ABI30_0_0YGBaseline(baselineChild);
  return baseline + baselineChild->getLayout().position[ABI30_0_0YGEdgeTop];
}

static bool ABI30_0_0YGIsBaselineLayout(const ABI30_0_0YGNodeRef node) {
  if (ABI30_0_0YGFlexDirectionIsColumn(node->getStyle().flexDirection)) {
    return false;
  }
  if (node->getStyle().alignItems == ABI30_0_0YGAlignBaseline) {
    return true;
  }
  const uint32_t childCount = ABI30_0_0YGNodeGetChildCount(node);
  for (uint32_t i = 0; i < childCount; i++) {
    const ABI30_0_0YGNodeRef child = ABI30_0_0YGNodeGetChild(node, i);
    if (child->getStyle().positionType == ABI30_0_0YGPositionTypeRelative &&
        child->getStyle().alignSelf == ABI30_0_0YGAlignBaseline) {
      return true;
    }
  }

  return false;
}

static inline float ABI30_0_0YGNodeDimWithMargin(const ABI30_0_0YGNodeRef node,
                                        const ABI30_0_0YGFlexDirection axis,
                                        const float widthSize) {
  return node->getLayout().measuredDimensions[dim[axis]] +
      node->getLeadingMargin(axis, widthSize) +
      node->getTrailingMargin(axis, widthSize);
}

static inline bool ABI30_0_0YGNodeIsStyleDimDefined(const ABI30_0_0YGNodeRef node,
                                           const ABI30_0_0YGFlexDirection axis,
                                           const float parentSize) {
  bool isUndefined =
      ABI30_0_0YGFloatIsUndefined(node->getResolvedDimension(dim[axis]).value);
  return !(
      node->getResolvedDimension(dim[axis]).unit == ABI30_0_0YGUnitAuto ||
      node->getResolvedDimension(dim[axis]).unit == ABI30_0_0YGUnitUndefined ||
      (node->getResolvedDimension(dim[axis]).unit == ABI30_0_0YGUnitPoint &&
       !isUndefined && node->getResolvedDimension(dim[axis]).value < 0.0f) ||
      (node->getResolvedDimension(dim[axis]).unit == ABI30_0_0YGUnitPercent &&
       !isUndefined &&
       (node->getResolvedDimension(dim[axis]).value < 0.0f ||
        ABI30_0_0YGFloatIsUndefined(parentSize))));
}

static inline bool ABI30_0_0YGNodeIsLayoutDimDefined(const ABI30_0_0YGNodeRef node, const ABI30_0_0YGFlexDirection axis) {
  const float value = node->getLayout().measuredDimensions[dim[axis]];
  return !ABI30_0_0YGFloatIsUndefined(value) && value >= 0.0f;
}

static float ABI30_0_0YGNodeBoundAxisWithinMinAndMax(const ABI30_0_0YGNodeRef node,
                                            const ABI30_0_0YGFlexDirection axis,
                                            const float value,
                                            const float axisSize) {
  float min = ABI30_0_0YGUndefined;
  float max = ABI30_0_0YGUndefined;

  if (ABI30_0_0YGFlexDirectionIsColumn(axis)) {
    min = ABI30_0_0YGResolveValue(
        node->getStyle().minDimensions[ABI30_0_0YGDimensionHeight], axisSize);
    max = ABI30_0_0YGResolveValue(
        node->getStyle().maxDimensions[ABI30_0_0YGDimensionHeight], axisSize);
  } else if (ABI30_0_0YGFlexDirectionIsRow(axis)) {
    min = ABI30_0_0YGResolveValue(
        node->getStyle().minDimensions[ABI30_0_0YGDimensionWidth], axisSize);
    max = ABI30_0_0YGResolveValue(
        node->getStyle().maxDimensions[ABI30_0_0YGDimensionWidth], axisSize);
  }

  float boundValue = value;

  if (!ABI30_0_0YGFloatIsUndefined(max) && max >= 0.0f && boundValue > max) {
    boundValue = max;
  }

  if (!ABI30_0_0YGFloatIsUndefined(min) && min >= 0.0f && boundValue < min) {
    boundValue = min;
  }

  return boundValue;
}

// Like ABI30_0_0YGNodeBoundAxisWithinMinAndMax but also ensures that the value doesn't go
// below the
// padding and border amount.
static inline float ABI30_0_0YGNodeBoundAxis(const ABI30_0_0YGNodeRef node,
                                    const ABI30_0_0YGFlexDirection axis,
                                    const float value,
                                    const float axisSize,
                                    const float widthSize) {
  return ABI30_0_0YGFloatMax(
      ABI30_0_0YGNodeBoundAxisWithinMinAndMax(node, axis, value, axisSize),
      ABI30_0_0YGNodePaddingAndBorderForAxis(node, axis, widthSize));
}

static void ABI30_0_0YGNodeSetChildTrailingPosition(const ABI30_0_0YGNodeRef node,
                                           const ABI30_0_0YGNodeRef child,
                                           const ABI30_0_0YGFlexDirection axis) {
  const float size = child->getLayout().measuredDimensions[dim[axis]];
  child->setLayoutPosition(
      node->getLayout().measuredDimensions[dim[axis]] - size -
          child->getLayout().position[pos[axis]],
      trailing[axis]);
}

static void ABI30_0_0YGConstrainMaxSizeForMode(const ABI30_0_0YGNodeRef node,
                                      const enum ABI30_0_0YGFlexDirection axis,
                                      const float parentAxisSize,
                                      const float parentWidth,
                                      ABI30_0_0YGMeasureMode *mode,
                                      float *size) {
  const float maxSize =
      ABI30_0_0YGResolveValue(
          node->getStyle().maxDimensions[dim[axis]], parentAxisSize) +
      node->getMarginForAxis(axis, parentWidth);
  switch (*mode) {
    case ABI30_0_0YGMeasureModeExactly:
    case ABI30_0_0YGMeasureModeAtMost:
      *size = (ABI30_0_0YGFloatIsUndefined(maxSize) || *size < maxSize) ? *size : maxSize;
      break;
    case ABI30_0_0YGMeasureModeUndefined:
      if (!ABI30_0_0YGFloatIsUndefined(maxSize)) {
        *mode = ABI30_0_0YGMeasureModeAtMost;
        *size = maxSize;
      }
      break;
  }
}

static void ABI30_0_0YGNodeComputeFlexBasisForChild(const ABI30_0_0YGNodeRef node,
                                           const ABI30_0_0YGNodeRef child,
                                           const float width,
                                           const ABI30_0_0YGMeasureMode widthMode,
                                           const float height,
                                           const float parentWidth,
                                           const float parentHeight,
                                           const ABI30_0_0YGMeasureMode heightMode,
                                           const ABI30_0_0YGDirection direction,
                                           const ABI30_0_0YGConfigRef config) {
  const ABI30_0_0YGFlexDirection mainAxis =
      ABI30_0_0YGResolveFlexDirection(node->getStyle().flexDirection, direction);
  const bool isMainAxisRow = ABI30_0_0YGFlexDirectionIsRow(mainAxis);
  const float mainAxisSize = isMainAxisRow ? width : height;
  const float mainAxisParentSize = isMainAxisRow ? parentWidth : parentHeight;

  float childWidth;
  float childHeight;
  ABI30_0_0YGMeasureMode childWidthMeasureMode;
  ABI30_0_0YGMeasureMode childHeightMeasureMode;

  const float resolvedFlexBasis =
      ABI30_0_0YGResolveValue(child->resolveFlexBasisPtr(), mainAxisParentSize);
  const bool isRowStyleDimDefined = ABI30_0_0YGNodeIsStyleDimDefined(child, ABI30_0_0YGFlexDirectionRow, parentWidth);
  const bool isColumnStyleDimDefined =
      ABI30_0_0YGNodeIsStyleDimDefined(child, ABI30_0_0YGFlexDirectionColumn, parentHeight);

  if (!ABI30_0_0YGFloatIsUndefined(resolvedFlexBasis) && !ABI30_0_0YGFloatIsUndefined(mainAxisSize)) {
    if (ABI30_0_0YGFloatIsUndefined(child->getLayout().computedFlexBasis) ||
        (ABI30_0_0YGConfigIsExperimentalFeatureEnabled(
             child->getConfig(), ABI30_0_0YGExperimentalFeatureWebFlexBasis) &&
         child->getLayout().computedFlexBasisGeneration !=
             gCurrentGenerationCount)) {
      child->setLayoutComputedFlexBasis(ABI30_0_0YGFloatMax(
          resolvedFlexBasis,
          ABI30_0_0YGNodePaddingAndBorderForAxis(child, mainAxis, parentWidth)));
    }
  } else if (isMainAxisRow && isRowStyleDimDefined) {
    // The width is definite, so use that as the flex basis.
    child->setLayoutComputedFlexBasis(ABI30_0_0YGFloatMax(
        ABI30_0_0YGResolveValue(
            child->getResolvedDimension(ABI30_0_0YGDimensionWidth), parentWidth),
        ABI30_0_0YGNodePaddingAndBorderForAxis(child, ABI30_0_0YGFlexDirectionRow, parentWidth)));
  } else if (!isMainAxisRow && isColumnStyleDimDefined) {
    // The height is definite, so use that as the flex basis.
    child->setLayoutComputedFlexBasis(ABI30_0_0YGFloatMax(
        ABI30_0_0YGResolveValue(
            child->getResolvedDimension(ABI30_0_0YGDimensionHeight), parentHeight),
        ABI30_0_0YGNodePaddingAndBorderForAxis(
            child, ABI30_0_0YGFlexDirectionColumn, parentWidth)));
  } else {
    // Compute the flex basis and hypothetical main size (i.e. the clamped
    // flex basis).
    childWidth = ABI30_0_0YGUndefined;
    childHeight = ABI30_0_0YGUndefined;
    childWidthMeasureMode = ABI30_0_0YGMeasureModeUndefined;
    childHeightMeasureMode = ABI30_0_0YGMeasureModeUndefined;

    const float marginRow =
        child->getMarginForAxis(ABI30_0_0YGFlexDirectionRow, parentWidth);
    const float marginColumn =
        child->getMarginForAxis(ABI30_0_0YGFlexDirectionColumn, parentWidth);

    if (isRowStyleDimDefined) {
      childWidth =
          ABI30_0_0YGResolveValue(
              child->getResolvedDimension(ABI30_0_0YGDimensionWidth), parentWidth) +
          marginRow;
      childWidthMeasureMode = ABI30_0_0YGMeasureModeExactly;
    }
    if (isColumnStyleDimDefined) {
      childHeight =
          ABI30_0_0YGResolveValue(
              child->getResolvedDimension(ABI30_0_0YGDimensionHeight), parentHeight) +
          marginColumn;
      childHeightMeasureMode = ABI30_0_0YGMeasureModeExactly;
    }

    // The W3C spec doesn't say anything about the 'overflow' property,
    // but all major browsers appear to implement the following logic.
    if ((!isMainAxisRow && node->getStyle().overflow == ABI30_0_0YGOverflowScroll) ||
        node->getStyle().overflow != ABI30_0_0YGOverflowScroll) {
      if (ABI30_0_0YGFloatIsUndefined(childWidth) && !ABI30_0_0YGFloatIsUndefined(width)) {
        childWidth = width;
        childWidthMeasureMode = ABI30_0_0YGMeasureModeAtMost;
      }
    }

    if ((isMainAxisRow && node->getStyle().overflow == ABI30_0_0YGOverflowScroll) ||
        node->getStyle().overflow != ABI30_0_0YGOverflowScroll) {
      if (ABI30_0_0YGFloatIsUndefined(childHeight) && !ABI30_0_0YGFloatIsUndefined(height)) {
        childHeight = height;
        childHeightMeasureMode = ABI30_0_0YGMeasureModeAtMost;
      }
    }

    if (!ABI30_0_0YGFloatIsUndefined(child->getStyle().aspectRatio)) {
      if (!isMainAxisRow && childWidthMeasureMode == ABI30_0_0YGMeasureModeExactly) {
        childHeight = marginColumn +
            (childWidth - marginRow) / child->getStyle().aspectRatio;
        childHeightMeasureMode = ABI30_0_0YGMeasureModeExactly;
      } else if (isMainAxisRow && childHeightMeasureMode == ABI30_0_0YGMeasureModeExactly) {
        childWidth = marginRow +
            (childHeight - marginColumn) * child->getStyle().aspectRatio;
        childWidthMeasureMode = ABI30_0_0YGMeasureModeExactly;
      }
    }

    // If child has no defined size in the cross axis and is set to stretch,
    // set the cross
    // axis to be measured exactly with the available inner width

    const bool hasExactWidth = !ABI30_0_0YGFloatIsUndefined(width) && widthMode == ABI30_0_0YGMeasureModeExactly;
    const bool childWidthStretch = ABI30_0_0YGNodeAlignItem(node, child) == ABI30_0_0YGAlignStretch &&
                                   childWidthMeasureMode != ABI30_0_0YGMeasureModeExactly;
    if (!isMainAxisRow && !isRowStyleDimDefined && hasExactWidth && childWidthStretch) {
      childWidth = width;
      childWidthMeasureMode = ABI30_0_0YGMeasureModeExactly;
      if (!ABI30_0_0YGFloatIsUndefined(child->getStyle().aspectRatio)) {
        childHeight = (childWidth - marginRow) / child->getStyle().aspectRatio;
        childHeightMeasureMode = ABI30_0_0YGMeasureModeExactly;
      }
    }

    const bool hasExactHeight = !ABI30_0_0YGFloatIsUndefined(height) && heightMode == ABI30_0_0YGMeasureModeExactly;
    const bool childHeightStretch = ABI30_0_0YGNodeAlignItem(node, child) == ABI30_0_0YGAlignStretch &&
                                    childHeightMeasureMode != ABI30_0_0YGMeasureModeExactly;
    if (isMainAxisRow && !isColumnStyleDimDefined && hasExactHeight && childHeightStretch) {
      childHeight = height;
      childHeightMeasureMode = ABI30_0_0YGMeasureModeExactly;

      if (!ABI30_0_0YGFloatIsUndefined(child->getStyle().aspectRatio)) {
        childWidth =
            (childHeight - marginColumn) * child->getStyle().aspectRatio;
        childWidthMeasureMode = ABI30_0_0YGMeasureModeExactly;
      }
    }

    ABI30_0_0YGConstrainMaxSizeForMode(
        child, ABI30_0_0YGFlexDirectionRow, parentWidth, parentWidth, &childWidthMeasureMode, &childWidth);
    ABI30_0_0YGConstrainMaxSizeForMode(child,
                              ABI30_0_0YGFlexDirectionColumn,
                              parentHeight,
                              parentWidth,
                              &childHeightMeasureMode,
                              &childHeight);

    // Measure the child
    ABI30_0_0YGLayoutNodeInternal(child,
                         childWidth,
                         childHeight,
                         direction,
                         childWidthMeasureMode,
                         childHeightMeasureMode,
                         parentWidth,
                         parentHeight,
                         false,
                         "measure",
                         config);

    child->setLayoutComputedFlexBasis(ABI30_0_0YGFloatMax(
        child->getLayout().measuredDimensions[dim[mainAxis]],
        ABI30_0_0YGNodePaddingAndBorderForAxis(child, mainAxis, parentWidth)));
  }
  child->setLayoutComputedFlexBasisGeneration(gCurrentGenerationCount);
}

static void ABI30_0_0YGNodeAbsoluteLayoutChild(const ABI30_0_0YGNodeRef node,
                                      const ABI30_0_0YGNodeRef child,
                                      const float width,
                                      const ABI30_0_0YGMeasureMode widthMode,
                                      const float height,
                                      const ABI30_0_0YGDirection direction,
                                      const ABI30_0_0YGConfigRef config) {
  const ABI30_0_0YGFlexDirection mainAxis =
      ABI30_0_0YGResolveFlexDirection(node->getStyle().flexDirection, direction);
  const ABI30_0_0YGFlexDirection crossAxis = ABI30_0_0YGFlexDirectionCross(mainAxis, direction);
  const bool isMainAxisRow = ABI30_0_0YGFlexDirectionIsRow(mainAxis);

  float childWidth = ABI30_0_0YGUndefined;
  float childHeight = ABI30_0_0YGUndefined;
  ABI30_0_0YGMeasureMode childWidthMeasureMode = ABI30_0_0YGMeasureModeUndefined;
  ABI30_0_0YGMeasureMode childHeightMeasureMode = ABI30_0_0YGMeasureModeUndefined;

  const float marginRow = child->getMarginForAxis(ABI30_0_0YGFlexDirectionRow, width);
  const float marginColumn =
      child->getMarginForAxis(ABI30_0_0YGFlexDirectionColumn, width);

  if (ABI30_0_0YGNodeIsStyleDimDefined(child, ABI30_0_0YGFlexDirectionRow, width)) {
    childWidth =
        ABI30_0_0YGResolveValue(child->getResolvedDimension(ABI30_0_0YGDimensionWidth), width) +
        marginRow;
  } else {
    // If the child doesn't have a specified width, compute the width based
    // on the left/right
    // offsets if they're defined.
    if (child->isLeadingPositionDefined(ABI30_0_0YGFlexDirectionRow) &&
        child->isTrailingPosDefined(ABI30_0_0YGFlexDirectionRow)) {
      childWidth = node->getLayout().measuredDimensions[ABI30_0_0YGDimensionWidth] -
          (node->getLeadingBorder(ABI30_0_0YGFlexDirectionRow) +
           node->getTrailingBorder(ABI30_0_0YGFlexDirectionRow)) -
          (child->getLeadingPosition(ABI30_0_0YGFlexDirectionRow, width) +
           child->getTrailingPosition(ABI30_0_0YGFlexDirectionRow, width));
      childWidth = ABI30_0_0YGNodeBoundAxis(child, ABI30_0_0YGFlexDirectionRow, childWidth, width, width);
    }
  }

  if (ABI30_0_0YGNodeIsStyleDimDefined(child, ABI30_0_0YGFlexDirectionColumn, height)) {
    childHeight =
        ABI30_0_0YGResolveValue(child->getResolvedDimension(ABI30_0_0YGDimensionHeight), height) +
        marginColumn;
  } else {
    // If the child doesn't have a specified height, compute the height
    // based on the top/bottom
    // offsets if they're defined.
    if (child->isLeadingPositionDefined(ABI30_0_0YGFlexDirectionColumn) &&
        child->isTrailingPosDefined(ABI30_0_0YGFlexDirectionColumn)) {
      childHeight = node->getLayout().measuredDimensions[ABI30_0_0YGDimensionHeight] -
          (node->getLeadingBorder(ABI30_0_0YGFlexDirectionColumn) +
           node->getTrailingBorder(ABI30_0_0YGFlexDirectionColumn)) -
          (child->getLeadingPosition(ABI30_0_0YGFlexDirectionColumn, height) +
           child->getTrailingPosition(ABI30_0_0YGFlexDirectionColumn, height));
      childHeight = ABI30_0_0YGNodeBoundAxis(child, ABI30_0_0YGFlexDirectionColumn, childHeight, height, width);
    }
  }

  // Exactly one dimension needs to be defined for us to be able to do aspect ratio
  // calculation. One dimension being the anchor and the other being flexible.
  if (ABI30_0_0YGFloatIsUndefined(childWidth) ^ ABI30_0_0YGFloatIsUndefined(childHeight)) {
    if (!ABI30_0_0YGFloatIsUndefined(child->getStyle().aspectRatio)) {
      if (ABI30_0_0YGFloatIsUndefined(childWidth)) {
        childWidth = marginRow +
            (childHeight - marginColumn) * child->getStyle().aspectRatio;
      } else if (ABI30_0_0YGFloatIsUndefined(childHeight)) {
        childHeight = marginColumn +
            (childWidth - marginRow) / child->getStyle().aspectRatio;
      }
    }
  }

  // If we're still missing one or the other dimension, measure the content.
  if (ABI30_0_0YGFloatIsUndefined(childWidth) || ABI30_0_0YGFloatIsUndefined(childHeight)) {
    childWidthMeasureMode =
        ABI30_0_0YGFloatIsUndefined(childWidth) ? ABI30_0_0YGMeasureModeUndefined : ABI30_0_0YGMeasureModeExactly;
    childHeightMeasureMode =
        ABI30_0_0YGFloatIsUndefined(childHeight) ? ABI30_0_0YGMeasureModeUndefined : ABI30_0_0YGMeasureModeExactly;

    // If the size of the parent is defined then try to constrain the absolute child to that size
    // as well. This allows text within the absolute child to wrap to the size of its parent.
    // This is the same behavior as many browsers implement.
    if (!isMainAxisRow && ABI30_0_0YGFloatIsUndefined(childWidth) &&
        widthMode != ABI30_0_0YGMeasureModeUndefined && !ABI30_0_0YGFloatIsUndefined(width) &&
        width > 0) {
      childWidth = width;
      childWidthMeasureMode = ABI30_0_0YGMeasureModeAtMost;
    }

    ABI30_0_0YGLayoutNodeInternal(child,
                         childWidth,
                         childHeight,
                         direction,
                         childWidthMeasureMode,
                         childHeightMeasureMode,
                         childWidth,
                         childHeight,
                         false,
                         "abs-measure",
                         config);
    childWidth = child->getLayout().measuredDimensions[ABI30_0_0YGDimensionWidth] +
        child->getMarginForAxis(ABI30_0_0YGFlexDirectionRow, width);
    childHeight = child->getLayout().measuredDimensions[ABI30_0_0YGDimensionHeight] +
        child->getMarginForAxis(ABI30_0_0YGFlexDirectionColumn, width);
  }

  ABI30_0_0YGLayoutNodeInternal(child,
                       childWidth,
                       childHeight,
                       direction,
                       ABI30_0_0YGMeasureModeExactly,
                       ABI30_0_0YGMeasureModeExactly,
                       childWidth,
                       childHeight,
                       true,
                       "abs-layout",
                       config);

  if (child->isTrailingPosDefined(mainAxis) &&
      !child->isLeadingPositionDefined(mainAxis)) {
    child->setLayoutPosition(
        node->getLayout().measuredDimensions[dim[mainAxis]] -
            child->getLayout().measuredDimensions[dim[mainAxis]] -
            node->getTrailingBorder(mainAxis) -
            child->getTrailingMargin(mainAxis, width) -
            child->getTrailingPosition(
                mainAxis, isMainAxisRow ? width : height),
        leading[mainAxis]);
  } else if (
      !child->isLeadingPositionDefined(mainAxis) &&
      node->getStyle().justifyContent == ABI30_0_0YGJustifyCenter) {
    child->setLayoutPosition(
        (node->getLayout().measuredDimensions[dim[mainAxis]] -
         child->getLayout().measuredDimensions[dim[mainAxis]]) /
            2.0f,
        leading[mainAxis]);
  } else if (
      !child->isLeadingPositionDefined(mainAxis) &&
      node->getStyle().justifyContent == ABI30_0_0YGJustifyFlexEnd) {
    child->setLayoutPosition(
        (node->getLayout().measuredDimensions[dim[mainAxis]] -
         child->getLayout().measuredDimensions[dim[mainAxis]]),
        leading[mainAxis]);
  }

  if (child->isTrailingPosDefined(crossAxis) &&
      !child->isLeadingPositionDefined(crossAxis)) {
    child->setLayoutPosition(
        node->getLayout().measuredDimensions[dim[crossAxis]] -
            child->getLayout().measuredDimensions[dim[crossAxis]] -
            node->getTrailingBorder(crossAxis) -
            child->getTrailingMargin(crossAxis, width) -
            child->getTrailingPosition(
                crossAxis, isMainAxisRow ? height : width),
        leading[crossAxis]);

  } else if (
      !child->isLeadingPositionDefined(crossAxis) &&
      ABI30_0_0YGNodeAlignItem(node, child) == ABI30_0_0YGAlignCenter) {
    child->setLayoutPosition(
        (node->getLayout().measuredDimensions[dim[crossAxis]] -
         child->getLayout().measuredDimensions[dim[crossAxis]]) /
            2.0f,
        leading[crossAxis]);
  } else if (
      !child->isLeadingPositionDefined(crossAxis) &&
      ((ABI30_0_0YGNodeAlignItem(node, child) == ABI30_0_0YGAlignFlexEnd) ^
       (node->getStyle().flexWrap == ABI30_0_0YGWrapWrapReverse))) {
    child->setLayoutPosition(
        (node->getLayout().measuredDimensions[dim[crossAxis]] -
         child->getLayout().measuredDimensions[dim[crossAxis]]),
        leading[crossAxis]);
  }
}

static void ABI30_0_0YGNodeWithMeasureFuncSetMeasuredDimensions(const ABI30_0_0YGNodeRef node,
                                                       const float availableWidth,
                                                       const float availableHeight,
                                                       const ABI30_0_0YGMeasureMode widthMeasureMode,
                                                       const ABI30_0_0YGMeasureMode heightMeasureMode,
                                                       const float parentWidth,
                                                       const float parentHeight) {
  ABI30_0_0YGAssertWithNode(
      node,
      node->getMeasure() != nullptr,
      "Expected node to have custom measure function");

  const float paddingAndBorderAxisRow =
      ABI30_0_0YGNodePaddingAndBorderForAxis(node, ABI30_0_0YGFlexDirectionRow, availableWidth);
  const float paddingAndBorderAxisColumn =
      ABI30_0_0YGNodePaddingAndBorderForAxis(node, ABI30_0_0YGFlexDirectionColumn, availableWidth);
  const float marginAxisRow =
      node->getMarginForAxis(ABI30_0_0YGFlexDirectionRow, availableWidth);
  const float marginAxisColumn =
      node->getMarginForAxis(ABI30_0_0YGFlexDirectionColumn, availableWidth);

  // We want to make sure we don't call measure with negative size
  const float innerWidth = ABI30_0_0YGFloatIsUndefined(availableWidth)
      ? availableWidth
      : ABI30_0_0YGFloatMax(0, availableWidth - marginAxisRow - paddingAndBorderAxisRow);
  const float innerHeight = ABI30_0_0YGFloatIsUndefined(availableHeight)
      ? availableHeight
      : ABI30_0_0YGFloatMax(
            0, availableHeight - marginAxisColumn - paddingAndBorderAxisColumn);

  if (widthMeasureMode == ABI30_0_0YGMeasureModeExactly &&
      heightMeasureMode == ABI30_0_0YGMeasureModeExactly) {
    // Don't bother sizing the text if both dimensions are already defined.
    node->setLayoutMeasuredDimension(
        ABI30_0_0YGNodeBoundAxis(
            node,
            ABI30_0_0YGFlexDirectionRow,
            availableWidth - marginAxisRow,
            parentWidth,
            parentWidth),
        ABI30_0_0YGDimensionWidth);
    node->setLayoutMeasuredDimension(
        ABI30_0_0YGNodeBoundAxis(
            node,
            ABI30_0_0YGFlexDirectionColumn,
            availableHeight - marginAxisColumn,
            parentHeight,
            parentWidth),
        ABI30_0_0YGDimensionHeight);
  } else {
    // Measure the text under the current constraints.
    const ABI30_0_0YGSize measuredSize = node->getMeasure()(
        node, innerWidth, widthMeasureMode, innerHeight, heightMeasureMode);

    node->setLayoutMeasuredDimension(
        ABI30_0_0YGNodeBoundAxis(
            node,
            ABI30_0_0YGFlexDirectionRow,
            (widthMeasureMode == ABI30_0_0YGMeasureModeUndefined ||
             widthMeasureMode == ABI30_0_0YGMeasureModeAtMost)
                ? measuredSize.width + paddingAndBorderAxisRow
                : availableWidth - marginAxisRow,
            parentWidth,
            parentWidth),
        ABI30_0_0YGDimensionWidth);

    node->setLayoutMeasuredDimension(
        ABI30_0_0YGNodeBoundAxis(
            node,
            ABI30_0_0YGFlexDirectionColumn,
            (heightMeasureMode == ABI30_0_0YGMeasureModeUndefined ||
             heightMeasureMode == ABI30_0_0YGMeasureModeAtMost)
                ? measuredSize.height + paddingAndBorderAxisColumn
                : availableHeight - marginAxisColumn,
            parentHeight,
            parentWidth),
        ABI30_0_0YGDimensionHeight);
  }
}

// For nodes with no children, use the available values if they were provided,
// or the minimum size as indicated by the padding and border sizes.
static void ABI30_0_0YGNodeEmptyContainerSetMeasuredDimensions(const ABI30_0_0YGNodeRef node,
                                                      const float availableWidth,
                                                      const float availableHeight,
                                                      const ABI30_0_0YGMeasureMode widthMeasureMode,
                                                      const ABI30_0_0YGMeasureMode heightMeasureMode,
                                                      const float parentWidth,
                                                      const float parentHeight) {
  const float paddingAndBorderAxisRow =
      ABI30_0_0YGNodePaddingAndBorderForAxis(node, ABI30_0_0YGFlexDirectionRow, parentWidth);
  const float paddingAndBorderAxisColumn =
      ABI30_0_0YGNodePaddingAndBorderForAxis(node, ABI30_0_0YGFlexDirectionColumn, parentWidth);
  const float marginAxisRow =
      node->getMarginForAxis(ABI30_0_0YGFlexDirectionRow, parentWidth);
  const float marginAxisColumn =
      node->getMarginForAxis(ABI30_0_0YGFlexDirectionColumn, parentWidth);

  node->setLayoutMeasuredDimension(
      ABI30_0_0YGNodeBoundAxis(
          node,
          ABI30_0_0YGFlexDirectionRow,
          (widthMeasureMode == ABI30_0_0YGMeasureModeUndefined ||
           widthMeasureMode == ABI30_0_0YGMeasureModeAtMost)
              ? paddingAndBorderAxisRow
              : availableWidth - marginAxisRow,
          parentWidth,
          parentWidth),
      ABI30_0_0YGDimensionWidth);

  node->setLayoutMeasuredDimension(
      ABI30_0_0YGNodeBoundAxis(
          node,
          ABI30_0_0YGFlexDirectionColumn,
          (heightMeasureMode == ABI30_0_0YGMeasureModeUndefined ||
           heightMeasureMode == ABI30_0_0YGMeasureModeAtMost)
              ? paddingAndBorderAxisColumn
              : availableHeight - marginAxisColumn,
          parentHeight,
          parentWidth),
      ABI30_0_0YGDimensionHeight);
}

static bool ABI30_0_0YGNodeFixedSizeSetMeasuredDimensions(const ABI30_0_0YGNodeRef node,
                                                 const float availableWidth,
                                                 const float availableHeight,
                                                 const ABI30_0_0YGMeasureMode widthMeasureMode,
                                                 const ABI30_0_0YGMeasureMode heightMeasureMode,
                                                 const float parentWidth,
                                                 const float parentHeight) {
  if ((!ABI30_0_0YGFloatIsUndefined(availableWidth) &&
       widthMeasureMode == ABI30_0_0YGMeasureModeAtMost && availableWidth <= 0.0f) ||
      (!ABI30_0_0YGFloatIsUndefined(availableHeight) &&
       heightMeasureMode == ABI30_0_0YGMeasureModeAtMost && availableHeight <= 0.0f) ||
      (widthMeasureMode == ABI30_0_0YGMeasureModeExactly &&
       heightMeasureMode == ABI30_0_0YGMeasureModeExactly)) {
    const float marginAxisColumn =
        node->getMarginForAxis(ABI30_0_0YGFlexDirectionColumn, parentWidth);
    const float marginAxisRow =
        node->getMarginForAxis(ABI30_0_0YGFlexDirectionRow, parentWidth);

    node->setLayoutMeasuredDimension(
        ABI30_0_0YGNodeBoundAxis(
            node,
            ABI30_0_0YGFlexDirectionRow,
            ABI30_0_0YGFloatIsUndefined(availableWidth) ||
                    (widthMeasureMode == ABI30_0_0YGMeasureModeAtMost &&
                     availableWidth < 0.0f)
                ? 0.0f
                : availableWidth - marginAxisRow,
            parentWidth,
            parentWidth),
        ABI30_0_0YGDimensionWidth);

    node->setLayoutMeasuredDimension(
        ABI30_0_0YGNodeBoundAxis(
            node,
            ABI30_0_0YGFlexDirectionColumn,
            ABI30_0_0YGFloatIsUndefined(availableHeight) ||
                    (heightMeasureMode == ABI30_0_0YGMeasureModeAtMost &&
                     availableHeight < 0.0f)
                ? 0.0f
                : availableHeight - marginAxisColumn,
            parentHeight,
            parentWidth),
        ABI30_0_0YGDimensionHeight);
    return true;
  }

  return false;
}

static void ABI30_0_0YGZeroOutLayoutRecursivly(const ABI30_0_0YGNodeRef node) {
  memset(&(node->getLayout()), 0, sizeof(ABI30_0_0YGLayout));
  node->setHasNewLayout(true);
  node->cloneChildrenIfNeeded();
  const uint32_t childCount = ABI30_0_0YGNodeGetChildCount(node);
  for (uint32_t i = 0; i < childCount; i++) {
    const ABI30_0_0YGNodeRef child = node->getChild(i);
    ABI30_0_0YGZeroOutLayoutRecursivly(child);
  }
}

static float ABI30_0_0YGNodeCalculateAvailableInnerDim(
    const ABI30_0_0YGNodeRef node,
    ABI30_0_0YGFlexDirection axis,
    float availableDim,
    float parentDim) {
  ABI30_0_0YGFlexDirection direction =
      ABI30_0_0YGFlexDirectionIsRow(axis) ? ABI30_0_0YGFlexDirectionRow : ABI30_0_0YGFlexDirectionColumn;
  ABI30_0_0YGDimension dimension =
      ABI30_0_0YGFlexDirectionIsRow(axis) ? ABI30_0_0YGDimensionWidth : ABI30_0_0YGDimensionHeight;

  const float margin = node->getMarginForAxis(direction, parentDim);
  const float paddingAndBorder =
      ABI30_0_0YGNodePaddingAndBorderForAxis(node, direction, parentDim);

  float availableInnerDim = availableDim - margin - paddingAndBorder;
  // Max dimension overrides predefined dimension value; Min dimension in turn
  // overrides both of the above
  if (!ABI30_0_0YGFloatIsUndefined(availableInnerDim)) {
    // We want to make sure our available height does not violate min and max
    // constraints
    const float minInnerDim =
        ABI30_0_0YGFloatIsUndefined(ABI30_0_0YGResolveValue(
            node->getStyle().minDimensions[dimension], parentDim))
        ? 0.0f
        : ABI30_0_0YGResolveValue(node->getStyle().minDimensions[dimension], parentDim) -
            paddingAndBorder;
    const float maxInnerDim =
        ABI30_0_0YGResolveValue(node->getStyle().maxDimensions[dimension], parentDim) -
        paddingAndBorder;
    availableInnerDim =
        ABI30_0_0YGFloatMax(ABI30_0_0YGFloatMin(availableInnerDim, maxInnerDim), minInnerDim);
  }

  return availableInnerDim;
}

static void ABI30_0_0YGNodeComputeFlexBasisForChildren(
    const ABI30_0_0YGNodeRef node,
    const float availableInnerWidth,
    const float availableInnerHeight,
    ABI30_0_0YGMeasureMode widthMeasureMode,
    ABI30_0_0YGMeasureMode heightMeasureMode,
    ABI30_0_0YGDirection direction,
    ABI30_0_0YGFlexDirection mainAxis,
    const ABI30_0_0YGConfigRef config,
    bool performLayout,
    float& totalOuterFlexBasis) {
  ABI30_0_0YGNodeRef singleFlexChild = nullptr;
  ABI30_0_0YGVector children = node->getChildren();
  ABI30_0_0YGMeasureMode measureModeMainDim =
      ABI30_0_0YGFlexDirectionIsRow(mainAxis) ? widthMeasureMode : heightMeasureMode;
  // If there is only one child with flexGrow + flexShrink it means we can set
  // the computedFlexBasis to 0 instead of measuring and shrinking / flexing the
  // child to exactly match the remaining space
  if (measureModeMainDim == ABI30_0_0YGMeasureModeExactly) {
    for (auto child : children) {
      if (singleFlexChild != nullptr) {
        if (child->isNodeFlexible()) {
          // There is already a flexible child, abort
          singleFlexChild = nullptr;
          break;
        }
      } else if (
          child->resolveFlexGrow() > 0.0f &&
          child->resolveFlexShrink() > 0.0f) {
        singleFlexChild = child;
      }
    }
  }

  for (auto child : children) {
    child->resolveDimension();
    if (child->getStyle().display == ABI30_0_0YGDisplayNone) {
      ABI30_0_0YGZeroOutLayoutRecursivly(child);
      child->setHasNewLayout(true);
      child->setDirty(false);
      continue;
    }
    if (performLayout) {
      // Set the initial position (relative to the parent).
      const ABI30_0_0YGDirection childDirection = child->resolveDirection(direction);
      const float mainDim = ABI30_0_0YGFlexDirectionIsRow(mainAxis)
          ? availableInnerWidth
          : availableInnerHeight;
      const float crossDim = ABI30_0_0YGFlexDirectionIsRow(mainAxis)
          ? availableInnerHeight
          : availableInnerWidth;
      child->setPosition(
          childDirection, mainDim, crossDim, availableInnerWidth);
    }

    if (child->getStyle().positionType == ABI30_0_0YGPositionTypeAbsolute) {
      continue;
    }
    if (child == singleFlexChild) {
      child->setLayoutComputedFlexBasisGeneration(gCurrentGenerationCount);
      child->setLayoutComputedFlexBasis(0);
    } else {
      ABI30_0_0YGNodeComputeFlexBasisForChild(
          node,
          child,
          availableInnerWidth,
          widthMeasureMode,
          availableInnerHeight,
          availableInnerWidth,
          availableInnerHeight,
          heightMeasureMode,
          direction,
          config);
    }

    totalOuterFlexBasis += child->getLayout().computedFlexBasis +
        child->getMarginForAxis(mainAxis, availableInnerWidth);
  }
}

// This function assumes that all the children of node have their
// computedFlexBasis properly computed(To do this use
// ABI30_0_0YGNodeComputeFlexBasisForChildren function).
// This function calculates ABI30_0_0YGCollectFlexItemsRowMeasurement
static ABI30_0_0YGCollectFlexItemsRowValues ABI30_0_0YGCalculateCollectFlexItemsRowValues(
    const ABI30_0_0YGNodeRef& node,
    const ABI30_0_0YGDirection parentDirection,
    const float mainAxisParentSize,
    const float availableInnerWidth,
    const float availableInnerMainDim,
    const uint32_t startOfLineIndex,
    const uint32_t lineCount) {
  ABI30_0_0YGCollectFlexItemsRowValues flexAlgoRowMeasurement = {};
  flexAlgoRowMeasurement.relativeChildren.reserve(node->getChildren().size());

  float sizeConsumedOnCurrentLineIncludingMinConstraint = 0;
  const ABI30_0_0YGFlexDirection mainAxis = ABI30_0_0YGResolveFlexDirection(
      node->getStyle().flexDirection, node->resolveDirection(parentDirection));
  const bool isNodeFlexWrap = node->getStyle().flexWrap != ABI30_0_0YGWrapNoWrap;

  // Add items to the current line until it's full or we run out of items.
  uint32_t endOfLineIndex = startOfLineIndex;
  for (; endOfLineIndex < node->getChildrenCount(); endOfLineIndex++) {
    const ABI30_0_0YGNodeRef child = node->getChild(endOfLineIndex);
    if (child->getStyle().display == ABI30_0_0YGDisplayNone ||
        child->getStyle().positionType == ABI30_0_0YGPositionTypeAbsolute) {
      continue;
    }
    child->setLineIndex(lineCount);
    const float childMarginMainAxis =
        child->getMarginForAxis(mainAxis, availableInnerWidth);
    const float flexBasisWithMinAndMaxConstraints =
        ABI30_0_0YGNodeBoundAxisWithinMinAndMax(
            child,
            mainAxis,
            child->getLayout().computedFlexBasis,
            mainAxisParentSize);

    // If this is a multi-line flow and this item pushes us over the
    // available size, we've
    // hit the end of the current line. Break out of the loop and lay out
    // the current line.
    if (sizeConsumedOnCurrentLineIncludingMinConstraint +
                flexBasisWithMinAndMaxConstraints + childMarginMainAxis >
            availableInnerMainDim &&
        isNodeFlexWrap && flexAlgoRowMeasurement.itemsOnLine > 0) {
      break;
    }

    sizeConsumedOnCurrentLineIncludingMinConstraint +=
        flexBasisWithMinAndMaxConstraints + childMarginMainAxis;
    flexAlgoRowMeasurement.sizeConsumedOnCurrentLine +=
        flexBasisWithMinAndMaxConstraints + childMarginMainAxis;
    flexAlgoRowMeasurement.itemsOnLine++;

    if (child->isNodeFlexible()) {
      flexAlgoRowMeasurement.totalFlexGrowFactors += child->resolveFlexGrow();

      // Unlike the grow factor, the shrink factor is scaled relative to the
      // child dimension.
      flexAlgoRowMeasurement.totalFlexShrinkScaledFactors +=
          -child->resolveFlexShrink() * child->getLayout().computedFlexBasis;
    }

    flexAlgoRowMeasurement.relativeChildren.push_back(child);
  }

  // The total flex factor needs to be floored to 1.
  if (flexAlgoRowMeasurement.totalFlexGrowFactors > 0 &&
      flexAlgoRowMeasurement.totalFlexGrowFactors < 1) {
    flexAlgoRowMeasurement.totalFlexGrowFactors = 1;
  }

  // The total flex shrink factor needs to be floored to 1.
  if (flexAlgoRowMeasurement.totalFlexShrinkScaledFactors > 0 &&
      flexAlgoRowMeasurement.totalFlexShrinkScaledFactors < 1) {
    flexAlgoRowMeasurement.totalFlexShrinkScaledFactors = 1;
  }
  flexAlgoRowMeasurement.endOfLineIndex = endOfLineIndex;
  return flexAlgoRowMeasurement;
}

// It distributes the free space to the flexible items and ensures that the size
// of the flex items abide the min and max constraints. At the end of this
// function the child nodes would have proper size. Prior using this function
// please ensure that ABI30_0_0YGDistributeFreeSpaceFirstPass is called.
static float ABI30_0_0YGDistributeFreeSpaceSecondPass(
    ABI30_0_0YGCollectFlexItemsRowValues& collectedFlexItemsValues,
    const ABI30_0_0YGNodeRef node,
    const ABI30_0_0YGFlexDirection mainAxis,
    const ABI30_0_0YGFlexDirection crossAxis,
    const float mainAxisParentSize,
    const float availableInnerMainDim,
    const float availableInnerCrossDim,
    const float availableInnerWidth,
    const float availableInnerHeight,
    const bool flexBasisOverflows,
    const ABI30_0_0YGMeasureMode measureModeCrossDim,
    const bool performLayout,
    const ABI30_0_0YGConfigRef config) {
  float childFlexBasis = 0;
  float flexShrinkScaledFactor = 0;
  float flexGrowFactor = 0;
  float deltaFreeSpace = 0;
  const bool isMainAxisRow = ABI30_0_0YGFlexDirectionIsRow(mainAxis);
  const bool isNodeFlexWrap = node->getStyle().flexWrap != ABI30_0_0YGWrapNoWrap;

  for (auto currentRelativeChild : collectedFlexItemsValues.relativeChildren) {
    childFlexBasis = ABI30_0_0YGNodeBoundAxisWithinMinAndMax(
        currentRelativeChild,
        mainAxis,
        currentRelativeChild->getLayout().computedFlexBasis,
        mainAxisParentSize);
    float updatedMainSize = childFlexBasis;

    if (!ABI30_0_0YGFloatIsUndefined(collectedFlexItemsValues.remainingFreeSpace) &&
        collectedFlexItemsValues.remainingFreeSpace < 0) {
      flexShrinkScaledFactor =
          -currentRelativeChild->resolveFlexShrink() * childFlexBasis;
      // Is this child able to shrink?
      if (flexShrinkScaledFactor != 0) {
        float childSize;

        if (!ABI30_0_0YGFloatIsUndefined(
                collectedFlexItemsValues.totalFlexShrinkScaledFactors) &&
            collectedFlexItemsValues.totalFlexShrinkScaledFactors == 0) {
          childSize = childFlexBasis + flexShrinkScaledFactor;
        } else {
          childSize = childFlexBasis +
              (collectedFlexItemsValues.remainingFreeSpace /
               collectedFlexItemsValues.totalFlexShrinkScaledFactors) *
                  flexShrinkScaledFactor;
        }

        updatedMainSize = ABI30_0_0YGNodeBoundAxis(
            currentRelativeChild,
            mainAxis,
            childSize,
            availableInnerMainDim,
            availableInnerWidth);
      }
    } else if (
        !ABI30_0_0YGFloatIsUndefined(collectedFlexItemsValues.remainingFreeSpace) &&
        collectedFlexItemsValues.remainingFreeSpace > 0) {
      flexGrowFactor = currentRelativeChild->resolveFlexGrow();

      // Is this child able to grow?
      if (!ABI30_0_0YGFloatIsUndefined(flexGrowFactor) && flexGrowFactor != 0) {
        updatedMainSize = ABI30_0_0YGNodeBoundAxis(
            currentRelativeChild,
            mainAxis,
            childFlexBasis +
                collectedFlexItemsValues.remainingFreeSpace /
                    collectedFlexItemsValues.totalFlexGrowFactors *
                    flexGrowFactor,
            availableInnerMainDim,
            availableInnerWidth);
      }
    }

    deltaFreeSpace += updatedMainSize - childFlexBasis;

    const float marginMain =
        currentRelativeChild->getMarginForAxis(mainAxis, availableInnerWidth);
    const float marginCross =
        currentRelativeChild->getMarginForAxis(crossAxis, availableInnerWidth);

    float childCrossSize;
    float childMainSize = updatedMainSize + marginMain;
    ABI30_0_0YGMeasureMode childCrossMeasureMode;
    ABI30_0_0YGMeasureMode childMainMeasureMode = ABI30_0_0YGMeasureModeExactly;

    if (!ABI30_0_0YGFloatIsUndefined(currentRelativeChild->getStyle().aspectRatio)) {
      childCrossSize = isMainAxisRow ? (childMainSize - marginMain) /
              currentRelativeChild->getStyle().aspectRatio
                                     : (childMainSize - marginMain) *
              currentRelativeChild->getStyle().aspectRatio;
      childCrossMeasureMode = ABI30_0_0YGMeasureModeExactly;

      childCrossSize += marginCross;
    } else if (
        !ABI30_0_0YGFloatIsUndefined(availableInnerCrossDim) &&
        !ABI30_0_0YGNodeIsStyleDimDefined(
            currentRelativeChild, crossAxis, availableInnerCrossDim) &&
        measureModeCrossDim == ABI30_0_0YGMeasureModeExactly &&
        !(isNodeFlexWrap && flexBasisOverflows) &&
        ABI30_0_0YGNodeAlignItem(node, currentRelativeChild) == ABI30_0_0YGAlignStretch &&
        currentRelativeChild->marginLeadingValue(crossAxis).unit !=
            ABI30_0_0YGUnitAuto &&
        currentRelativeChild->marginTrailingValue(crossAxis).unit !=
            ABI30_0_0YGUnitAuto) {
      childCrossSize = availableInnerCrossDim;
      childCrossMeasureMode = ABI30_0_0YGMeasureModeExactly;
    } else if (!ABI30_0_0YGNodeIsStyleDimDefined(
                   currentRelativeChild, crossAxis, availableInnerCrossDim)) {
      childCrossSize = availableInnerCrossDim;
      childCrossMeasureMode = ABI30_0_0YGFloatIsUndefined(childCrossSize)
          ? ABI30_0_0YGMeasureModeUndefined
          : ABI30_0_0YGMeasureModeAtMost;
    } else {
      childCrossSize =
          ABI30_0_0YGResolveValue(
              currentRelativeChild->getResolvedDimension(dim[crossAxis]),
              availableInnerCrossDim) +
          marginCross;
      const bool isLoosePercentageMeasurement =
          currentRelativeChild->getResolvedDimension(dim[crossAxis]).unit ==
              ABI30_0_0YGUnitPercent &&
          measureModeCrossDim != ABI30_0_0YGMeasureModeExactly;
      childCrossMeasureMode =
          ABI30_0_0YGFloatIsUndefined(childCrossSize) || isLoosePercentageMeasurement
          ? ABI30_0_0YGMeasureModeUndefined
          : ABI30_0_0YGMeasureModeExactly;
    }

    ABI30_0_0YGConstrainMaxSizeForMode(
        currentRelativeChild,
        mainAxis,
        availableInnerMainDim,
        availableInnerWidth,
        &childMainMeasureMode,
        &childMainSize);
    ABI30_0_0YGConstrainMaxSizeForMode(
        currentRelativeChild,
        crossAxis,
        availableInnerCrossDim,
        availableInnerWidth,
        &childCrossMeasureMode,
        &childCrossSize);

    const bool requiresStretchLayout =
        !ABI30_0_0YGNodeIsStyleDimDefined(
            currentRelativeChild, crossAxis, availableInnerCrossDim) &&
        ABI30_0_0YGNodeAlignItem(node, currentRelativeChild) == ABI30_0_0YGAlignStretch &&
        currentRelativeChild->marginLeadingValue(crossAxis).unit !=
            ABI30_0_0YGUnitAuto &&
        currentRelativeChild->marginTrailingValue(crossAxis).unit != ABI30_0_0YGUnitAuto;

    const float childWidth = isMainAxisRow ? childMainSize : childCrossSize;
    const float childHeight = !isMainAxisRow ? childMainSize : childCrossSize;

    const ABI30_0_0YGMeasureMode childWidthMeasureMode =
        isMainAxisRow ? childMainMeasureMode : childCrossMeasureMode;
    const ABI30_0_0YGMeasureMode childHeightMeasureMode =
        !isMainAxisRow ? childMainMeasureMode : childCrossMeasureMode;

    // Recursively call the layout algorithm for this child with the updated
    // main size.
    ABI30_0_0YGLayoutNodeInternal(
        currentRelativeChild,
        childWidth,
        childHeight,
        node->getLayout().direction,
        childWidthMeasureMode,
        childHeightMeasureMode,
        availableInnerWidth,
        availableInnerHeight,
        performLayout && !requiresStretchLayout,
        "flex",
        config);
    node->setLayoutHadOverflow(
        node->getLayout().hadOverflow |
        currentRelativeChild->getLayout().hadOverflow);
  }
  return deltaFreeSpace;
}

// It distributes the free space to the flexible items.For those flexible items
// whose min and max constraints are triggered, those flex item's clamped size
// is removed from the remaingfreespace.
static void ABI30_0_0YGDistributeFreeSpaceFirstPass(
    ABI30_0_0YGCollectFlexItemsRowValues& collectedFlexItemsValues,
    const ABI30_0_0YGFlexDirection mainAxis,
    const float mainAxisParentSize,
    const float availableInnerMainDim,
    const float availableInnerWidth) {
  float flexShrinkScaledFactor = 0;
  float flexGrowFactor = 0;
  float baseMainSize = 0;
  float boundMainSize = 0;
  float deltaFreeSpace = 0;

  for (auto currentRelativeChild : collectedFlexItemsValues.relativeChildren) {
    float childFlexBasis = ABI30_0_0YGNodeBoundAxisWithinMinAndMax(
        currentRelativeChild,
        mainAxis,
        currentRelativeChild->getLayout().computedFlexBasis,
        mainAxisParentSize);

    if (collectedFlexItemsValues.remainingFreeSpace < 0) {
      flexShrinkScaledFactor =
          -currentRelativeChild->resolveFlexShrink() * childFlexBasis;

      // Is this child able to shrink?
      if (!ABI30_0_0YGFloatIsUndefined(flexShrinkScaledFactor) &&
          flexShrinkScaledFactor != 0) {
        baseMainSize = childFlexBasis +
            collectedFlexItemsValues.remainingFreeSpace /
                collectedFlexItemsValues.totalFlexShrinkScaledFactors *
                flexShrinkScaledFactor;
        boundMainSize = ABI30_0_0YGNodeBoundAxis(
            currentRelativeChild,
            mainAxis,
            baseMainSize,
            availableInnerMainDim,
            availableInnerWidth);
        if (!ABI30_0_0YGFloatIsUndefined(baseMainSize) &&
            !ABI30_0_0YGFloatIsUndefined(boundMainSize) &&
            baseMainSize != boundMainSize) {
          // By excluding this item's size and flex factor from remaining,
          // this item's
          // min/max constraints should also trigger in the second pass
          // resulting in the
          // item's size calculation being identical in the first and second
          // passes.
          deltaFreeSpace += boundMainSize - childFlexBasis;
          collectedFlexItemsValues.totalFlexShrinkScaledFactors -=
              flexShrinkScaledFactor;
        }
      }
    } else if (
        !ABI30_0_0YGFloatIsUndefined(collectedFlexItemsValues.remainingFreeSpace) &&
        collectedFlexItemsValues.remainingFreeSpace > 0) {
      flexGrowFactor = currentRelativeChild->resolveFlexGrow();

      // Is this child able to grow?
      if (!ABI30_0_0YGFloatIsUndefined(flexGrowFactor) && flexGrowFactor != 0) {
        baseMainSize = childFlexBasis +
            collectedFlexItemsValues.remainingFreeSpace /
                collectedFlexItemsValues.totalFlexGrowFactors * flexGrowFactor;
        boundMainSize = ABI30_0_0YGNodeBoundAxis(
            currentRelativeChild,
            mainAxis,
            baseMainSize,
            availableInnerMainDim,
            availableInnerWidth);

        if (!ABI30_0_0YGFloatIsUndefined(baseMainSize) &&
            !ABI30_0_0YGFloatIsUndefined(boundMainSize) &&
            baseMainSize != boundMainSize) {
          // By excluding this item's size and flex factor from remaining,
          // this item's
          // min/max constraints should also trigger in the second pass
          // resulting in the
          // item's size calculation being identical in the first and second
          // passes.
          deltaFreeSpace += boundMainSize - childFlexBasis;
          collectedFlexItemsValues.totalFlexGrowFactors -= flexGrowFactor;
        }
      }
    }
  }
  collectedFlexItemsValues.remainingFreeSpace -= deltaFreeSpace;
}

// Do two passes over the flex items to figure out how to distribute the
// remaining space.
// The first pass finds the items whose min/max constraints trigger,
// freezes them at those
// sizes, and excludes those sizes from the remaining space. The second
// pass sets the size
// of each flexible item. It distributes the remaining space amongst the
// items whose min/max
// constraints didn't trigger in pass 1. For the other items, it sets
// their sizes by forcing
// their min/max constraints to trigger again.
//
// This two pass approach for resolving min/max constraints deviates from
// the spec. The
// spec (https://www.w3.org/TR/YG-flexbox-1/#resolve-flexible-lengths)
// describes a process
// that needs to be repeated a variable number of times. The algorithm
// implemented here
// won't handle all cases but it was simpler to implement and it mitigates
// performance
// concerns because we know exactly how many passes it'll do.
//
// At the end of this function the child nodes would have the proper size
// assigned to them.
//
static void ABI30_0_0YGResolveFlexibleLength(
    const ABI30_0_0YGNodeRef node,
    ABI30_0_0YGCollectFlexItemsRowValues& collectedFlexItemsValues,
    const ABI30_0_0YGFlexDirection mainAxis,
    const ABI30_0_0YGFlexDirection crossAxis,
    const float mainAxisParentSize,
    const float availableInnerMainDim,
    const float availableInnerCrossDim,
    const float availableInnerWidth,
    const float availableInnerHeight,
    const bool flexBasisOverflows,
    const ABI30_0_0YGMeasureMode measureModeCrossDim,
    const bool performLayout,
    const ABI30_0_0YGConfigRef config) {
  const float originalFreeSpace = collectedFlexItemsValues.remainingFreeSpace;
  // First pass: detect the flex items whose min/max constraints trigger
  ABI30_0_0YGDistributeFreeSpaceFirstPass(
      collectedFlexItemsValues,
      mainAxis,
      mainAxisParentSize,
      availableInnerMainDim,
      availableInnerWidth);

  // Second pass: resolve the sizes of the flexible items
  const float distributedFreeSpace = ABI30_0_0YGDistributeFreeSpaceSecondPass(
      collectedFlexItemsValues,
      node,
      mainAxis,
      crossAxis,
      mainAxisParentSize,
      availableInnerMainDim,
      availableInnerCrossDim,
      availableInnerWidth,
      availableInnerHeight,
      flexBasisOverflows,
      measureModeCrossDim,
      performLayout,
      config);

  collectedFlexItemsValues.remainingFreeSpace =
      originalFreeSpace - distributedFreeSpace;
}

static void ABI30_0_0YGJustifyMainAxis(
    const ABI30_0_0YGNodeRef node,
    ABI30_0_0YGCollectFlexItemsRowValues& collectedFlexItemsValues,
    const uint32_t& startOfLineIndex,
    const ABI30_0_0YGFlexDirection& mainAxis,
    const ABI30_0_0YGFlexDirection& crossAxis,
    const ABI30_0_0YGMeasureMode& measureModeMainDim,
    const ABI30_0_0YGMeasureMode& measureModeCrossDim,
    const float& mainAxisParentSize,
    const float& parentWidth,
    const float& availableInnerMainDim,
    const float& availableInnerCrossDim,
    const float& availableInnerWidth,
    const bool& performLayout) {
  const ABI30_0_0YGStyle style = node->getStyle();

  // If we are using "at most" rules in the main axis. Calculate the remaining
  // space when constraint by the min size defined for the main axis.
  if (measureModeMainDim == ABI30_0_0YGMeasureModeAtMost &&
      collectedFlexItemsValues.remainingFreeSpace > 0) {
    if (style.minDimensions[dim[mainAxis]].unit != ABI30_0_0YGUnitUndefined &&
        !ABI30_0_0YGFloatIsUndefined(ABI30_0_0YGResolveValue(
            style.minDimensions[dim[mainAxis]], mainAxisParentSize))) {
      collectedFlexItemsValues.remainingFreeSpace = ABI30_0_0YGFloatMax(
          0,
          ABI30_0_0YGResolveValue(
              style.minDimensions[dim[mainAxis]], mainAxisParentSize) -
              (availableInnerMainDim -
               collectedFlexItemsValues.remainingFreeSpace));
    } else {
      collectedFlexItemsValues.remainingFreeSpace = 0;
    }
  }

  int numberOfAutoMarginsOnCurrentLine = 0;
  for (uint32_t i = startOfLineIndex;
       i < collectedFlexItemsValues.endOfLineIndex;
       i++) {
    const ABI30_0_0YGNodeRef child = node->getChild(i);
    if (child->getStyle().positionType == ABI30_0_0YGPositionTypeRelative) {
      if (child->marginLeadingValue(mainAxis).unit == ABI30_0_0YGUnitAuto) {
        numberOfAutoMarginsOnCurrentLine++;
      }
      if (child->marginTrailingValue(mainAxis).unit == ABI30_0_0YGUnitAuto) {
        numberOfAutoMarginsOnCurrentLine++;
      }
    }
  }

  // In order to position the elements in the main axis, we have two
  // controls. The space between the beginning and the first element
  // and the space between each two elements.
  float leadingMainDim = 0;
  float betweenMainDim = 0;
  const ABI30_0_0YGJustify justifyContent = node->getStyle().justifyContent;

  if (numberOfAutoMarginsOnCurrentLine == 0) {
    switch (justifyContent) {
      case ABI30_0_0YGJustifyCenter:
        leadingMainDim = collectedFlexItemsValues.remainingFreeSpace / 2;
        break;
      case ABI30_0_0YGJustifyFlexEnd:
        leadingMainDim = collectedFlexItemsValues.remainingFreeSpace;
        break;
      case ABI30_0_0YGJustifySpaceBetween:
        if (collectedFlexItemsValues.itemsOnLine > 1) {
          betweenMainDim =
              ABI30_0_0YGFloatMax(collectedFlexItemsValues.remainingFreeSpace, 0) /
              (collectedFlexItemsValues.itemsOnLine - 1);
        } else {
          betweenMainDim = 0;
        }
        break;
      case ABI30_0_0YGJustifySpaceEvenly:
        // Space is distributed evenly across all elements
        betweenMainDim = collectedFlexItemsValues.remainingFreeSpace /
            (collectedFlexItemsValues.itemsOnLine + 1);
        leadingMainDim = betweenMainDim;
        break;
      case ABI30_0_0YGJustifySpaceAround:
        // Space on the edges is half of the space between elements
        betweenMainDim = collectedFlexItemsValues.remainingFreeSpace /
            collectedFlexItemsValues.itemsOnLine;
        leadingMainDim = betweenMainDim / 2;
        break;
      case ABI30_0_0YGJustifyFlexStart:
        break;
    }
  }

  const float leadingPaddingAndBorderMain =
      node->getLeadingPaddingAndBorder(mainAxis, parentWidth);
  collectedFlexItemsValues.mainDim =
      leadingPaddingAndBorderMain + leadingMainDim;
  collectedFlexItemsValues.crossDim = 0;

  for (uint32_t i = startOfLineIndex;
       i < collectedFlexItemsValues.endOfLineIndex;
       i++) {
    const ABI30_0_0YGNodeRef child = node->getChild(i);
    const ABI30_0_0YGStyle childStyle = child->getStyle();
    const ABI30_0_0YGLayout childLayout = child->getLayout();
    if (childStyle.display == ABI30_0_0YGDisplayNone) {
      continue;
    }
    if (childStyle.positionType == ABI30_0_0YGPositionTypeAbsolute &&
        child->isLeadingPositionDefined(mainAxis)) {
      if (performLayout) {
        // In case the child is position absolute and has left/top being
        // defined, we override the position to whatever the user said
        // (and margin/border).
        child->setLayoutPosition(
            child->getLeadingPosition(mainAxis, availableInnerMainDim) +
                node->getLeadingBorder(mainAxis) +
                child->getLeadingMargin(mainAxis, availableInnerWidth),
            pos[mainAxis]);
      }
    } else {
      // Now that we placed the element, we need to update the variables.
      // We need to do that only for relative elements. Absolute elements
      // do not take part in that phase.
      if (childStyle.positionType == ABI30_0_0YGPositionTypeRelative) {
        if (child->marginLeadingValue(mainAxis).unit == ABI30_0_0YGUnitAuto) {
          collectedFlexItemsValues.mainDim +=
              collectedFlexItemsValues.remainingFreeSpace /
              numberOfAutoMarginsOnCurrentLine;
        }

        if (performLayout) {
          child->setLayoutPosition(
              childLayout.position[pos[mainAxis]] +
                  collectedFlexItemsValues.mainDim,
              pos[mainAxis]);
        }

        if (child->marginTrailingValue(mainAxis).unit == ABI30_0_0YGUnitAuto) {
          collectedFlexItemsValues.mainDim +=
              collectedFlexItemsValues.remainingFreeSpace /
              numberOfAutoMarginsOnCurrentLine;
        }
        bool canSkipFlex =
            !performLayout && measureModeCrossDim == ABI30_0_0YGMeasureModeExactly;
        if (canSkipFlex) {
          // If we skipped the flex step, then we can't rely on the
          // measuredDims because
          // they weren't computed. This means we can't call
          // ABI30_0_0YGNodeDimWithMargin.
          collectedFlexItemsValues.mainDim += betweenMainDim +
              child->getMarginForAxis(mainAxis, availableInnerWidth) +
              childLayout.computedFlexBasis;
          collectedFlexItemsValues.crossDim = availableInnerCrossDim;
        } else {
          // The main dimension is the sum of all the elements dimension plus
          // the spacing.
          collectedFlexItemsValues.mainDim += betweenMainDim +
              ABI30_0_0YGNodeDimWithMargin(child, mainAxis, availableInnerWidth);

          // The cross dimension is the max of the elements dimension since
          // there can only be one element in that cross dimension.
          collectedFlexItemsValues.crossDim = ABI30_0_0YGFloatMax(
              collectedFlexItemsValues.crossDim,
              ABI30_0_0YGNodeDimWithMargin(child, crossAxis, availableInnerWidth));
        }
      } else if (performLayout) {
        child->setLayoutPosition(
            childLayout.position[pos[mainAxis]] +
                node->getLeadingBorder(mainAxis) + leadingMainDim,
            pos[mainAxis]);
      }
    }
  }
  collectedFlexItemsValues.mainDim +=
      node->getTrailingPaddingAndBorder(mainAxis, parentWidth);
}

//
// This is the main routine that implements a subset of the flexbox layout
// algorithm
// described in the W3C ABI30_0_0YG documentation: https://www.w3.org/TR/YG3-flexbox/.
//
// Limitations of this algorithm, compared to the full standard:
//  * Display property is always assumed to be 'flex' except for Text nodes,
//  which
//    are assumed to be 'inline-flex'.
//  * The 'zIndex' property (or any form of z ordering) is not supported. Nodes
//  are
//    stacked in document order.
//  * The 'order' property is not supported. The order of flex items is always
//  defined
//    by document order.
//  * The 'visibility' property is always assumed to be 'visible'. Values of
//  'collapse'
//    and 'hidden' are not supported.
//  * There is no support for forced breaks.
//  * It does not support vertical inline directions (top-to-bottom or
//  bottom-to-top text).
//
// Deviations from standard:
//  * Section 4.5 of the spec indicates that all flex items have a default
//  minimum
//    main size. For text blocks, for example, this is the width of the widest
//    word.
//    Calculating the minimum width is expensive, so we forego it and assume a
//    default
//    minimum main size of 0.
//  * Min/Max sizes in the main axis are not honored when resolving flexible
//  lengths.
//  * The spec indicates that the default value for 'flexDirection' is 'row',
//  but
//    the algorithm below assumes a default of 'column'.
//
// Input parameters:
//    - node: current node to be sized and layed out
//    - availableWidth & availableHeight: available size to be used for sizing
//    the node
//      or ABI30_0_0YGUndefined if the size is not available; interpretation depends on
//      layout
//      flags
//    - parentDirection: the inline (text) direction within the parent
//    (left-to-right or
//      right-to-left)
//    - widthMeasureMode: indicates the sizing rules for the width (see below
//    for explanation)
//    - heightMeasureMode: indicates the sizing rules for the height (see below
//    for explanation)
//    - performLayout: specifies whether the caller is interested in just the
//    dimensions
//      of the node or it requires the entire node and its subtree to be layed
//      out
//      (with final positions)
//
// Details:
//    This routine is called recursively to lay out subtrees of flexbox
//    elements. It uses the
//    information in node.style, which is treated as a read-only input. It is
//    responsible for
//    setting the layout.direction and layout.measuredDimensions fields for the
//    input node as well
//    as the layout.position and layout.lineIndex fields for its child nodes.
//    The
//    layout.measuredDimensions field includes any border or padding for the
//    node but does
//    not include margins.
//
//    The spec describes four different layout modes: "fill available", "max
//    content", "min
//    content",
//    and "fit content". Of these, we don't use "min content" because we don't
//    support default
//    minimum main sizes (see above for details). Each of our measure modes maps
//    to a layout mode
//    from the spec (https://www.w3.org/TR/YG3-sizing/#terms):
//      - ABI30_0_0YGMeasureModeUndefined: max content
//      - ABI30_0_0YGMeasureModeExactly: fill available
//      - ABI30_0_0YGMeasureModeAtMost: fit content
//
//    When calling ABI30_0_0YGNodelayoutImpl and ABI30_0_0YGLayoutNodeInternal, if the caller passes
//    an available size of
//    undefined then it must also pass a measure mode of ABI30_0_0YGMeasureModeUndefined
//    in that dimension.
//
static void ABI30_0_0YGNodelayoutImpl(const ABI30_0_0YGNodeRef node,
                             const float availableWidth,
                             const float availableHeight,
                             const ABI30_0_0YGDirection parentDirection,
                             const ABI30_0_0YGMeasureMode widthMeasureMode,
                             const ABI30_0_0YGMeasureMode heightMeasureMode,
                             const float parentWidth,
                             const float parentHeight,
                             const bool performLayout,
                             const ABI30_0_0YGConfigRef config) {
  ABI30_0_0YGAssertWithNode(node,
                   ABI30_0_0YGFloatIsUndefined(availableWidth) ? widthMeasureMode == ABI30_0_0YGMeasureModeUndefined
                                                      : true,
                   "availableWidth is indefinite so widthMeasureMode must be "
                   "ABI30_0_0YGMeasureModeUndefined");
  ABI30_0_0YGAssertWithNode(node,
                   ABI30_0_0YGFloatIsUndefined(availableHeight) ? heightMeasureMode == ABI30_0_0YGMeasureModeUndefined
                                                       : true,
                   "availableHeight is indefinite so heightMeasureMode must be "
                   "ABI30_0_0YGMeasureModeUndefined");

  // Set the resolved resolution in the node's layout.
  const ABI30_0_0YGDirection direction = node->resolveDirection(parentDirection);
  node->setLayoutDirection(direction);

  const ABI30_0_0YGFlexDirection flexRowDirection = ABI30_0_0YGResolveFlexDirection(ABI30_0_0YGFlexDirectionRow, direction);
  const ABI30_0_0YGFlexDirection flexColumnDirection =
      ABI30_0_0YGResolveFlexDirection(ABI30_0_0YGFlexDirectionColumn, direction);

  node->setLayoutMargin(
      node->getLeadingMargin(flexRowDirection, parentWidth), ABI30_0_0YGEdgeStart);
  node->setLayoutMargin(
      node->getTrailingMargin(flexRowDirection, parentWidth), ABI30_0_0YGEdgeEnd);
  node->setLayoutMargin(
      node->getLeadingMargin(flexColumnDirection, parentWidth), ABI30_0_0YGEdgeTop);
  node->setLayoutMargin(
      node->getTrailingMargin(flexColumnDirection, parentWidth), ABI30_0_0YGEdgeBottom);

  node->setLayoutBorder(node->getLeadingBorder(flexRowDirection), ABI30_0_0YGEdgeStart);
  node->setLayoutBorder(node->getTrailingBorder(flexRowDirection), ABI30_0_0YGEdgeEnd);
  node->setLayoutBorder(node->getLeadingBorder(flexColumnDirection), ABI30_0_0YGEdgeTop);
  node->setLayoutBorder(
      node->getTrailingBorder(flexColumnDirection), ABI30_0_0YGEdgeBottom);

  node->setLayoutPadding(
      node->getLeadingPadding(flexRowDirection, parentWidth), ABI30_0_0YGEdgeStart);
  node->setLayoutPadding(
      node->getTrailingPadding(flexRowDirection, parentWidth), ABI30_0_0YGEdgeEnd);
  node->setLayoutPadding(
      node->getLeadingPadding(flexColumnDirection, parentWidth), ABI30_0_0YGEdgeTop);
  node->setLayoutPadding(
      node->getTrailingPadding(flexColumnDirection, parentWidth), ABI30_0_0YGEdgeBottom);

  if (node->getMeasure() != nullptr) {
    ABI30_0_0YGNodeWithMeasureFuncSetMeasuredDimensions(node,
                                               availableWidth,
                                               availableHeight,
                                               widthMeasureMode,
                                               heightMeasureMode,
                                               parentWidth,
                                               parentHeight);
    return;
  }

  const uint32_t childCount = ABI30_0_0YGNodeGetChildCount(node);
  if (childCount == 0) {
    ABI30_0_0YGNodeEmptyContainerSetMeasuredDimensions(node,
                                              availableWidth,
                                              availableHeight,
                                              widthMeasureMode,
                                              heightMeasureMode,
                                              parentWidth,
                                              parentHeight);
    return;
  }

  // If we're not being asked to perform a full layout we can skip the algorithm if we already know
  // the size
  if (!performLayout && ABI30_0_0YGNodeFixedSizeSetMeasuredDimensions(node,
                                                             availableWidth,
                                                             availableHeight,
                                                             widthMeasureMode,
                                                             heightMeasureMode,
                                                             parentWidth,
                                                             parentHeight)) {
    return;
  }

  // At this point we know we're going to perform work. Ensure that each child has a mutable copy.
  node->cloneChildrenIfNeeded();
  // Reset layout flags, as they could have changed.
  node->setLayoutHadOverflow(false);

  // STEP 1: CALCULATE VALUES FOR REMAINDER OF ALGORITHM
  const ABI30_0_0YGFlexDirection mainAxis =
      ABI30_0_0YGResolveFlexDirection(node->getStyle().flexDirection, direction);
  const ABI30_0_0YGFlexDirection crossAxis = ABI30_0_0YGFlexDirectionCross(mainAxis, direction);
  const bool isMainAxisRow = ABI30_0_0YGFlexDirectionIsRow(mainAxis);
  const bool isNodeFlexWrap = node->getStyle().flexWrap != ABI30_0_0YGWrapNoWrap;

  const float mainAxisParentSize = isMainAxisRow ? parentWidth : parentHeight;
  const float crossAxisParentSize = isMainAxisRow ? parentHeight : parentWidth;

  const float leadingPaddingAndBorderCross =
      node->getLeadingPaddingAndBorder(crossAxis, parentWidth);
  const float paddingAndBorderAxisMain = ABI30_0_0YGNodePaddingAndBorderForAxis(node, mainAxis, parentWidth);
  const float paddingAndBorderAxisCross =
      ABI30_0_0YGNodePaddingAndBorderForAxis(node, crossAxis, parentWidth);

  ABI30_0_0YGMeasureMode measureModeMainDim = isMainAxisRow ? widthMeasureMode : heightMeasureMode;
  ABI30_0_0YGMeasureMode measureModeCrossDim = isMainAxisRow ? heightMeasureMode : widthMeasureMode;

  const float paddingAndBorderAxisRow =
      isMainAxisRow ? paddingAndBorderAxisMain : paddingAndBorderAxisCross;
  const float paddingAndBorderAxisColumn =
      isMainAxisRow ? paddingAndBorderAxisCross : paddingAndBorderAxisMain;

  const float marginAxisRow =
      node->getMarginForAxis(ABI30_0_0YGFlexDirectionRow, parentWidth);
  const float marginAxisColumn =
      node->getMarginForAxis(ABI30_0_0YGFlexDirectionColumn, parentWidth);

  const float minInnerWidth =
      ABI30_0_0YGResolveValue(
          node->getStyle().minDimensions[ABI30_0_0YGDimensionWidth], parentWidth) -
      paddingAndBorderAxisRow;
  const float maxInnerWidth =
      ABI30_0_0YGResolveValue(
          node->getStyle().maxDimensions[ABI30_0_0YGDimensionWidth], parentWidth) -
      paddingAndBorderAxisRow;
  const float minInnerHeight =
      ABI30_0_0YGResolveValue(
          node->getStyle().minDimensions[ABI30_0_0YGDimensionHeight], parentHeight) -
      paddingAndBorderAxisColumn;
  const float maxInnerHeight =
      ABI30_0_0YGResolveValue(
          node->getStyle().maxDimensions[ABI30_0_0YGDimensionHeight], parentHeight) -
      paddingAndBorderAxisColumn;

  const float minInnerMainDim = isMainAxisRow ? minInnerWidth : minInnerHeight;
  const float maxInnerMainDim = isMainAxisRow ? maxInnerWidth : maxInnerHeight;

  // STEP 2: DETERMINE AVAILABLE SIZE IN MAIN AND CROSS DIRECTIONS

  float availableInnerWidth = ABI30_0_0YGNodeCalculateAvailableInnerDim(
      node, ABI30_0_0YGFlexDirectionRow, availableWidth, parentWidth);
  float availableInnerHeight = ABI30_0_0YGNodeCalculateAvailableInnerDim(
      node, ABI30_0_0YGFlexDirectionColumn, availableHeight, parentHeight);

  float availableInnerMainDim =
      isMainAxisRow ? availableInnerWidth : availableInnerHeight;
  const float availableInnerCrossDim =
      isMainAxisRow ? availableInnerHeight : availableInnerWidth;

  float totalOuterFlexBasis = 0;

  // STEP 3: DETERMINE FLEX BASIS FOR EACH ITEM

  ABI30_0_0YGNodeComputeFlexBasisForChildren(
      node,
      availableInnerWidth,
      availableInnerHeight,
      widthMeasureMode,
      heightMeasureMode,
      direction,
      mainAxis,
      config,
      performLayout,
      totalOuterFlexBasis);

  const bool flexBasisOverflows = measureModeMainDim == ABI30_0_0YGMeasureModeUndefined
      ? false
      : totalOuterFlexBasis > availableInnerMainDim;
  if (isNodeFlexWrap && flexBasisOverflows &&
      measureModeMainDim == ABI30_0_0YGMeasureModeAtMost) {
    measureModeMainDim = ABI30_0_0YGMeasureModeExactly;
  }
  // STEP 4: COLLECT FLEX ITEMS INTO FLEX LINES

  // Indexes of children that represent the first and last items in the line.
  uint32_t startOfLineIndex = 0;
  uint32_t endOfLineIndex = 0;

  // Number of lines.
  uint32_t lineCount = 0;

  // Accumulated cross dimensions of all lines so far.
  float totalLineCrossDim = 0;

  // Max main dimension of all the lines.
  float maxLineMainDim = 0;
  ABI30_0_0YGCollectFlexItemsRowValues collectedFlexItemsValues;
  for (; endOfLineIndex < childCount;
       lineCount++, startOfLineIndex = endOfLineIndex) {
    collectedFlexItemsValues = ABI30_0_0YGCalculateCollectFlexItemsRowValues(
        node,
        parentDirection,
        mainAxisParentSize,
        availableInnerWidth,
        availableInnerMainDim,
        startOfLineIndex,
        lineCount);
    endOfLineIndex = collectedFlexItemsValues.endOfLineIndex;

    // If we don't need to measure the cross axis, we can skip the entire flex
    // step.
    const bool canSkipFlex =
        !performLayout && measureModeCrossDim == ABI30_0_0YGMeasureModeExactly;

    // STEP 5: RESOLVING FLEXIBLE LENGTHS ON MAIN AXIS
    // Calculate the remaining available space that needs to be allocated.
    // If the main dimension size isn't known, it is computed based on
    // the line length, so there's no more space left to distribute.

    bool sizeBasedOnContent = false;
    // If we don't measure with exact main dimension we want to ensure we don't violate min and max
    if (measureModeMainDim != ABI30_0_0YGMeasureModeExactly) {
      if (!ABI30_0_0YGFloatIsUndefined(minInnerMainDim) &&
          collectedFlexItemsValues.sizeConsumedOnCurrentLine <
              minInnerMainDim) {
        availableInnerMainDim = minInnerMainDim;
      } else if (
          !ABI30_0_0YGFloatIsUndefined(maxInnerMainDim) &&
          collectedFlexItemsValues.sizeConsumedOnCurrentLine >
              maxInnerMainDim) {
        availableInnerMainDim = maxInnerMainDim;
      } else {
        if (!node->getConfig()->useLegacyStretchBehaviour &&
            ((ABI30_0_0YGFloatIsUndefined(
                  collectedFlexItemsValues.totalFlexGrowFactors) &&
              collectedFlexItemsValues.totalFlexGrowFactors == 0) ||
             (ABI30_0_0YGFloatIsUndefined(node->resolveFlexGrow()) &&
              node->resolveFlexGrow() == 0))) {
          // If we don't have any children to flex or we can't flex the node
          // itself, space we've used is all space we need. Root node also
          // should be shrunk to minimum
          availableInnerMainDim =
              collectedFlexItemsValues.sizeConsumedOnCurrentLine;
        }

        if (node->getConfig()->useLegacyStretchBehaviour) {
          node->setLayoutDidUseLegacyFlag(true);
        }
        sizeBasedOnContent = !node->getConfig()->useLegacyStretchBehaviour;
      }
    }

    if (!sizeBasedOnContent && !ABI30_0_0YGFloatIsUndefined(availableInnerMainDim)) {
      collectedFlexItemsValues.remainingFreeSpace = availableInnerMainDim -
          collectedFlexItemsValues.sizeConsumedOnCurrentLine;
    } else if (collectedFlexItemsValues.sizeConsumedOnCurrentLine < 0) {
      // availableInnerMainDim is indefinite which means the node is being sized based on its
      // content.
      // sizeConsumedOnCurrentLine is negative which means the node will allocate 0 points for
      // its content. Consequently, remainingFreeSpace is 0 - sizeConsumedOnCurrentLine.
      collectedFlexItemsValues.remainingFreeSpace =
          -collectedFlexItemsValues.sizeConsumedOnCurrentLine;
    }

    if (!canSkipFlex) {
      ABI30_0_0YGResolveFlexibleLength(
          node,
          collectedFlexItemsValues,
          mainAxis,
          crossAxis,
          mainAxisParentSize,
          availableInnerMainDim,
          availableInnerCrossDim,
          availableInnerWidth,
          availableInnerHeight,
          flexBasisOverflows,
          measureModeCrossDim,
          performLayout,
          config);
    }

    node->setLayoutHadOverflow(
        node->getLayout().hadOverflow |
        (collectedFlexItemsValues.remainingFreeSpace < 0));

    // STEP 6: MAIN-AXIS JUSTIFICATION & CROSS-AXIS SIZE DETERMINATION

    // At this point, all the children have their dimensions set in the main
    // axis.
    // Their dimensions are also set in the cross axis with the exception of
    // items
    // that are aligned "stretch". We need to compute these stretch values and
    // set the final positions.

    ABI30_0_0YGJustifyMainAxis(
        node,
        collectedFlexItemsValues,
        startOfLineIndex,
        mainAxis,
        crossAxis,
        measureModeMainDim,
        measureModeCrossDim,
        mainAxisParentSize,
        parentWidth,
        availableInnerMainDim,
        availableInnerCrossDim,
        availableInnerWidth,
        performLayout);

    float containerCrossAxis = availableInnerCrossDim;
    if (measureModeCrossDim == ABI30_0_0YGMeasureModeUndefined ||
        measureModeCrossDim == ABI30_0_0YGMeasureModeAtMost) {
      // Compute the cross axis from the max cross dimension of the children.
      containerCrossAxis =
          ABI30_0_0YGNodeBoundAxis(
              node,
              crossAxis,
              collectedFlexItemsValues.crossDim + paddingAndBorderAxisCross,
              crossAxisParentSize,
              parentWidth) -
          paddingAndBorderAxisCross;
    }

    // If there's no flex wrap, the cross dimension is defined by the container.
    if (!isNodeFlexWrap && measureModeCrossDim == ABI30_0_0YGMeasureModeExactly) {
      collectedFlexItemsValues.crossDim = availableInnerCrossDim;
    }

    // Clamp to the min/max size specified on the container.
    collectedFlexItemsValues.crossDim =
        ABI30_0_0YGNodeBoundAxis(
            node,
            crossAxis,
            collectedFlexItemsValues.crossDim + paddingAndBorderAxisCross,
            crossAxisParentSize,
            parentWidth) -
        paddingAndBorderAxisCross;

    // STEP 7: CROSS-AXIS ALIGNMENT
    // We can skip child alignment if we're just measuring the container.
    if (performLayout) {
      for (uint32_t i = startOfLineIndex; i < endOfLineIndex; i++) {
        const ABI30_0_0YGNodeRef child = node->getChild(i);
        if (child->getStyle().display == ABI30_0_0YGDisplayNone) {
          continue;
        }
        if (child->getStyle().positionType == ABI30_0_0YGPositionTypeAbsolute) {
          // If the child is absolutely positioned and has a
          // top/left/bottom/right set, override
          // all the previously computed positions to set it correctly.
          const bool isChildLeadingPosDefined =
              child->isLeadingPositionDefined(crossAxis);
          if (isChildLeadingPosDefined) {
            child->setLayoutPosition(
                child->getLeadingPosition(crossAxis, availableInnerCrossDim) +
                    node->getLeadingBorder(crossAxis) +
                    child->getLeadingMargin(crossAxis, availableInnerWidth),
                pos[crossAxis]);
          }
          // If leading position is not defined or calculations result in Nan, default to border + margin
          if (!isChildLeadingPosDefined ||
              ABI30_0_0YGFloatIsUndefined(child->getLayout().position[pos[crossAxis]])) {
            child->setLayoutPosition(
                node->getLeadingBorder(crossAxis) +
                    child->getLeadingMargin(crossAxis, availableInnerWidth),
                pos[crossAxis]);
          }
        } else {
          float leadingCrossDim = leadingPaddingAndBorderCross;

          // For a relative children, we're either using alignItems (parent) or
          // alignSelf (child) in order to determine the position in the cross
          // axis
          const ABI30_0_0YGAlign alignItem = ABI30_0_0YGNodeAlignItem(node, child);

          // If the child uses align stretch, we need to lay it out one more
          // time, this time
          // forcing the cross-axis size to be the computed cross size for the
          // current line.
          if (alignItem == ABI30_0_0YGAlignStretch &&
              child->marginLeadingValue(crossAxis).unit != ABI30_0_0YGUnitAuto &&
              child->marginTrailingValue(crossAxis).unit != ABI30_0_0YGUnitAuto) {
            // If the child defines a definite size for its cross axis, there's
            // no need to stretch.
            if (!ABI30_0_0YGNodeIsStyleDimDefined(child, crossAxis, availableInnerCrossDim)) {
              float childMainSize =
                  child->getLayout().measuredDimensions[dim[mainAxis]];
              float childCrossSize =
                  !ABI30_0_0YGFloatIsUndefined(child->getStyle().aspectRatio)
                  ? ((child->getMarginForAxis(crossAxis, availableInnerWidth) +
                      (isMainAxisRow
                           ? childMainSize / child->getStyle().aspectRatio
                           : childMainSize * child->getStyle().aspectRatio)))
                  : collectedFlexItemsValues.crossDim;

              childMainSize +=
                  child->getMarginForAxis(mainAxis, availableInnerWidth);

              ABI30_0_0YGMeasureMode childMainMeasureMode = ABI30_0_0YGMeasureModeExactly;
              ABI30_0_0YGMeasureMode childCrossMeasureMode = ABI30_0_0YGMeasureModeExactly;
              ABI30_0_0YGConstrainMaxSizeForMode(child,
                                        mainAxis,
                                        availableInnerMainDim,
                                        availableInnerWidth,
                                        &childMainMeasureMode,
                                        &childMainSize);
              ABI30_0_0YGConstrainMaxSizeForMode(child,
                                        crossAxis,
                                        availableInnerCrossDim,
                                        availableInnerWidth,
                                        &childCrossMeasureMode,
                                        &childCrossSize);

              const float childWidth = isMainAxisRow ? childMainSize : childCrossSize;
              const float childHeight = !isMainAxisRow ? childMainSize : childCrossSize;

              const ABI30_0_0YGMeasureMode childWidthMeasureMode =
                  ABI30_0_0YGFloatIsUndefined(childWidth) ? ABI30_0_0YGMeasureModeUndefined
                                                 : ABI30_0_0YGMeasureModeExactly;
              const ABI30_0_0YGMeasureMode childHeightMeasureMode =
                  ABI30_0_0YGFloatIsUndefined(childHeight) ? ABI30_0_0YGMeasureModeUndefined
                                                  : ABI30_0_0YGMeasureModeExactly;

              ABI30_0_0YGLayoutNodeInternal(
                  child,
                  childWidth,
                  childHeight,
                  direction,
                  childWidthMeasureMode,
                  childHeightMeasureMode,
                  availableInnerWidth,
                  availableInnerHeight,
                  true,
                  "stretch",
                  config);
            }
          } else {
            const float remainingCrossDim = containerCrossAxis -
                ABI30_0_0YGNodeDimWithMargin(child, crossAxis, availableInnerWidth);

            if (child->marginLeadingValue(crossAxis).unit == ABI30_0_0YGUnitAuto &&
                child->marginTrailingValue(crossAxis).unit == ABI30_0_0YGUnitAuto) {
              leadingCrossDim += ABI30_0_0YGFloatMax(0.0f, remainingCrossDim / 2);
            } else if (
                child->marginTrailingValue(crossAxis).unit == ABI30_0_0YGUnitAuto) {
              // No-Op
            } else if (
                child->marginLeadingValue(crossAxis).unit == ABI30_0_0YGUnitAuto) {
              leadingCrossDim += ABI30_0_0YGFloatMax(0.0f, remainingCrossDim);
            } else if (alignItem == ABI30_0_0YGAlignFlexStart) {
              // No-Op
            } else if (alignItem == ABI30_0_0YGAlignCenter) {
              leadingCrossDim += remainingCrossDim / 2;
            } else {
              leadingCrossDim += remainingCrossDim;
            }
          }
          // And we apply the position
          child->setLayoutPosition(
              child->getLayout().position[pos[crossAxis]] + totalLineCrossDim +
                  leadingCrossDim,
              pos[crossAxis]);
        }
      }
    }

    totalLineCrossDim += collectedFlexItemsValues.crossDim;
    maxLineMainDim =
        ABI30_0_0YGFloatMax(maxLineMainDim, collectedFlexItemsValues.mainDim);
  }

  // STEP 8: MULTI-LINE CONTENT ALIGNMENT
  if (performLayout && (lineCount > 1 || ABI30_0_0YGIsBaselineLayout(node)) &&
      !ABI30_0_0YGFloatIsUndefined(availableInnerCrossDim)) {
    const float remainingAlignContentDim = availableInnerCrossDim - totalLineCrossDim;

    float crossDimLead = 0;
    float currentLead = leadingPaddingAndBorderCross;

    switch (node->getStyle().alignContent) {
      case ABI30_0_0YGAlignFlexEnd:
        currentLead += remainingAlignContentDim;
        break;
      case ABI30_0_0YGAlignCenter:
        currentLead += remainingAlignContentDim / 2;
        break;
      case ABI30_0_0YGAlignStretch:
        if (availableInnerCrossDim > totalLineCrossDim) {
          crossDimLead = remainingAlignContentDim / lineCount;
        }
        break;
      case ABI30_0_0YGAlignSpaceAround:
        if (availableInnerCrossDim > totalLineCrossDim) {
          currentLead += remainingAlignContentDim / (2 * lineCount);
          if (lineCount > 1) {
            crossDimLead = remainingAlignContentDim / lineCount;
          }
        } else {
          currentLead += remainingAlignContentDim / 2;
        }
        break;
      case ABI30_0_0YGAlignSpaceBetween:
        if (availableInnerCrossDim > totalLineCrossDim && lineCount > 1) {
          crossDimLead = remainingAlignContentDim / (lineCount - 1);
        }
        break;
      case ABI30_0_0YGAlignAuto:
      case ABI30_0_0YGAlignFlexStart:
      case ABI30_0_0YGAlignBaseline:
        break;
    }

    uint32_t endIndex = 0;
    for (uint32_t i = 0; i < lineCount; i++) {
      const uint32_t startIndex = endIndex;
      uint32_t ii;

      // compute the line's height and find the endIndex
      float lineHeight = 0;
      float maxAscentForCurrentLine = 0;
      float maxDescentForCurrentLine = 0;
      for (ii = startIndex; ii < childCount; ii++) {
        const ABI30_0_0YGNodeRef child = node->getChild(ii);
        if (child->getStyle().display == ABI30_0_0YGDisplayNone) {
          continue;
        }
        if (child->getStyle().positionType == ABI30_0_0YGPositionTypeRelative) {
          if (child->getLineIndex() != i) {
            break;
          }
          if (ABI30_0_0YGNodeIsLayoutDimDefined(child, crossAxis)) {
            lineHeight = ABI30_0_0YGFloatMax(
                lineHeight,
                child->getLayout().measuredDimensions[dim[crossAxis]] +
                    child->getMarginForAxis(crossAxis, availableInnerWidth));
          }
          if (ABI30_0_0YGNodeAlignItem(node, child) == ABI30_0_0YGAlignBaseline) {
            const float ascent = ABI30_0_0YGBaseline(child) +
                child->getLeadingMargin(
                    ABI30_0_0YGFlexDirectionColumn, availableInnerWidth);
            const float descent =
                child->getLayout().measuredDimensions[ABI30_0_0YGDimensionHeight] +
                child->getMarginForAxis(
                    ABI30_0_0YGFlexDirectionColumn, availableInnerWidth) -
                ascent;
            maxAscentForCurrentLine =
                ABI30_0_0YGFloatMax(maxAscentForCurrentLine, ascent);
            maxDescentForCurrentLine =
                ABI30_0_0YGFloatMax(maxDescentForCurrentLine, descent);
            lineHeight = ABI30_0_0YGFloatMax(
                lineHeight, maxAscentForCurrentLine + maxDescentForCurrentLine);
          }
        }
      }
      endIndex = ii;
      lineHeight += crossDimLead;

      if (performLayout) {
        for (ii = startIndex; ii < endIndex; ii++) {
          const ABI30_0_0YGNodeRef child = node->getChild(ii);
          if (child->getStyle().display == ABI30_0_0YGDisplayNone) {
            continue;
          }
          if (child->getStyle().positionType == ABI30_0_0YGPositionTypeRelative) {
            switch (ABI30_0_0YGNodeAlignItem(node, child)) {
              case ABI30_0_0YGAlignFlexStart: {
                child->setLayoutPosition(
                    currentLead +
                        child->getLeadingMargin(crossAxis, availableInnerWidth),
                    pos[crossAxis]);
                break;
              }
              case ABI30_0_0YGAlignFlexEnd: {
                child->setLayoutPosition(
                    currentLead + lineHeight -
                        child->getTrailingMargin(
                            crossAxis, availableInnerWidth) -
                        child->getLayout().measuredDimensions[dim[crossAxis]],
                    pos[crossAxis]);
                break;
              }
              case ABI30_0_0YGAlignCenter: {
                float childHeight =
                    child->getLayout().measuredDimensions[dim[crossAxis]];

                child->setLayoutPosition(
                    currentLead + (lineHeight - childHeight) / 2,
                    pos[crossAxis]);
                break;
              }
              case ABI30_0_0YGAlignStretch: {
                child->setLayoutPosition(
                    currentLead +
                        child->getLeadingMargin(crossAxis, availableInnerWidth),
                    pos[crossAxis]);

                // Remeasure child with the line height as it as been only measured with the
                // parents height yet.
                if (!ABI30_0_0YGNodeIsStyleDimDefined(child, crossAxis, availableInnerCrossDim)) {
                  const float childWidth = isMainAxisRow
                      ? (child->getLayout()
                             .measuredDimensions[ABI30_0_0YGDimensionWidth] +
                         child->getMarginForAxis(mainAxis, availableInnerWidth))
                      : lineHeight;

                  const float childHeight = !isMainAxisRow
                      ? (child->getLayout()
                             .measuredDimensions[ABI30_0_0YGDimensionHeight] +
                         child->getMarginForAxis(
                             crossAxis, availableInnerWidth))
                      : lineHeight;

                  if (!(ABI30_0_0YGFloatsEqual(
                            childWidth,
                            child->getLayout()
                                .measuredDimensions[ABI30_0_0YGDimensionWidth]) &&
                        ABI30_0_0YGFloatsEqual(
                            childHeight,
                            child->getLayout()
                                .measuredDimensions[ABI30_0_0YGDimensionHeight]))) {
                    ABI30_0_0YGLayoutNodeInternal(child,
                                         childWidth,
                                         childHeight,
                                         direction,
                                         ABI30_0_0YGMeasureModeExactly,
                                         ABI30_0_0YGMeasureModeExactly,
                                         availableInnerWidth,
                                         availableInnerHeight,
                                         true,
                                         "multiline-stretch",
                                         config);
                  }
                }
                break;
              }
              case ABI30_0_0YGAlignBaseline: {
                child->setLayoutPosition(
                    currentLead + maxAscentForCurrentLine - ABI30_0_0YGBaseline(child) +
                        child->getLeadingPosition(
                            ABI30_0_0YGFlexDirectionColumn, availableInnerCrossDim),
                    ABI30_0_0YGEdgeTop);

                break;
              }
              case ABI30_0_0YGAlignAuto:
              case ABI30_0_0YGAlignSpaceBetween:
              case ABI30_0_0YGAlignSpaceAround:
                break;
            }
          }
        }
      }

      currentLead += lineHeight;
    }
  }

  // STEP 9: COMPUTING FINAL DIMENSIONS

  node->setLayoutMeasuredDimension(
      ABI30_0_0YGNodeBoundAxis(
          node,
          ABI30_0_0YGFlexDirectionRow,
          availableWidth - marginAxisRow,
          parentWidth,
          parentWidth),
      ABI30_0_0YGDimensionWidth);

  node->setLayoutMeasuredDimension(
      ABI30_0_0YGNodeBoundAxis(
          node,
          ABI30_0_0YGFlexDirectionColumn,
          availableHeight - marginAxisColumn,
          parentHeight,
          parentWidth),
      ABI30_0_0YGDimensionHeight);

  // If the user didn't specify a width or height for the node, set the
  // dimensions based on the children.
  if (measureModeMainDim == ABI30_0_0YGMeasureModeUndefined ||
      (node->getStyle().overflow != ABI30_0_0YGOverflowScroll &&
       measureModeMainDim == ABI30_0_0YGMeasureModeAtMost)) {
    // Clamp the size to the min/max size, if specified, and make sure it
    // doesn't go below the padding and border amount.
    node->setLayoutMeasuredDimension(
        ABI30_0_0YGNodeBoundAxis(
            node, mainAxis, maxLineMainDim, mainAxisParentSize, parentWidth),
        dim[mainAxis]);

  } else if (
      measureModeMainDim == ABI30_0_0YGMeasureModeAtMost &&
      node->getStyle().overflow == ABI30_0_0YGOverflowScroll) {
    node->setLayoutMeasuredDimension(
        ABI30_0_0YGFloatMax(
            ABI30_0_0YGFloatMin(
                availableInnerMainDim + paddingAndBorderAxisMain,
                ABI30_0_0YGNodeBoundAxisWithinMinAndMax(
                    node, mainAxis, maxLineMainDim, mainAxisParentSize)),
            paddingAndBorderAxisMain),
        dim[mainAxis]);
  }

  if (measureModeCrossDim == ABI30_0_0YGMeasureModeUndefined ||
      (node->getStyle().overflow != ABI30_0_0YGOverflowScroll &&
       measureModeCrossDim == ABI30_0_0YGMeasureModeAtMost)) {
    // Clamp the size to the min/max size, if specified, and make sure it
    // doesn't go below the padding and border amount.

    node->setLayoutMeasuredDimension(
        ABI30_0_0YGNodeBoundAxis(
            node,
            crossAxis,
            totalLineCrossDim + paddingAndBorderAxisCross,
            crossAxisParentSize,
            parentWidth),
        dim[crossAxis]);

  } else if (
      measureModeCrossDim == ABI30_0_0YGMeasureModeAtMost &&
      node->getStyle().overflow == ABI30_0_0YGOverflowScroll) {
    node->setLayoutMeasuredDimension(
        ABI30_0_0YGFloatMax(
            ABI30_0_0YGFloatMin(
                availableInnerCrossDim + paddingAndBorderAxisCross,
                ABI30_0_0YGNodeBoundAxisWithinMinAndMax(
                    node,
                    crossAxis,
                    totalLineCrossDim + paddingAndBorderAxisCross,
                    crossAxisParentSize)),
            paddingAndBorderAxisCross),
        dim[crossAxis]);
  }

  // As we only wrapped in normal direction yet, we need to reverse the positions on wrap-reverse.
  if (performLayout && node->getStyle().flexWrap == ABI30_0_0YGWrapWrapReverse) {
    for (uint32_t i = 0; i < childCount; i++) {
      const ABI30_0_0YGNodeRef child = ABI30_0_0YGNodeGetChild(node, i);
      if (child->getStyle().positionType == ABI30_0_0YGPositionTypeRelative) {
        child->setLayoutPosition(
            node->getLayout().measuredDimensions[dim[crossAxis]] -
                child->getLayout().position[pos[crossAxis]] -
                child->getLayout().measuredDimensions[dim[crossAxis]],
            pos[crossAxis]);
      }
    }
  }

  if (performLayout) {
    // STEP 10: SIZING AND POSITIONING ABSOLUTE CHILDREN
    for (auto child : node->getChildren()) {
      if (child->getStyle().positionType != ABI30_0_0YGPositionTypeAbsolute) {
        continue;
      }
      ABI30_0_0YGNodeAbsoluteLayoutChild(
          node,
          child,
          availableInnerWidth,
          isMainAxisRow ? measureModeMainDim : measureModeCrossDim,
          availableInnerHeight,
          direction,
          config);
    }

    // STEP 11: SETTING TRAILING POSITIONS FOR CHILDREN
    const bool needsMainTrailingPos =
        mainAxis == ABI30_0_0YGFlexDirectionRowReverse || mainAxis == ABI30_0_0YGFlexDirectionColumnReverse;
    const bool needsCrossTrailingPos =
        crossAxis == ABI30_0_0YGFlexDirectionRowReverse || crossAxis == ABI30_0_0YGFlexDirectionColumnReverse;

    // Set trailing position if necessary.
    if (needsMainTrailingPos || needsCrossTrailingPos) {
      for (uint32_t i = 0; i < childCount; i++) {
        const ABI30_0_0YGNodeRef child = node->getChild(i);
        if (child->getStyle().display == ABI30_0_0YGDisplayNone) {
          continue;
        }
        if (needsMainTrailingPos) {
          ABI30_0_0YGNodeSetChildTrailingPosition(node, child, mainAxis);
        }

        if (needsCrossTrailingPos) {
          ABI30_0_0YGNodeSetChildTrailingPosition(node, child, crossAxis);
        }
      }
    }
  }
}

uint32_t gDepth = 0;
bool gPrintTree = false;
bool gPrintChanges = false;
bool gPrintSkips = false;

static const char *spacer = "                                                            ";

static const char *ABI30_0_0YGSpacer(const unsigned long level) {
  const size_t spacerLen = strlen(spacer);
  if (level > spacerLen) {
    return &spacer[0];
  } else {
    return &spacer[spacerLen - level];
  }
}

static const char *ABI30_0_0YGMeasureModeName(const ABI30_0_0YGMeasureMode mode, const bool performLayout) {
  const char *kMeasureModeNames[ABI30_0_0YGMeasureModeCount] = {"UNDEFINED", "ABI30_0_0EXACTLY", "AT_MOST"};
  const char *kLayoutModeNames[ABI30_0_0YGMeasureModeCount] = {"LAY_UNDEFINED",
                                                      "LAY_EXACTLY",
                                                      "LAY_AT_"
                                                      "MOST"};

  if (mode >= ABI30_0_0YGMeasureModeCount) {
    return "";
  }

  return performLayout ? kLayoutModeNames[mode] : kMeasureModeNames[mode];
}

static inline bool ABI30_0_0YGMeasureModeSizeIsExactAndMatchesOldMeasuredSize(ABI30_0_0YGMeasureMode sizeMode,
                                                                     float size,
                                                                     float lastComputedSize) {
  return sizeMode == ABI30_0_0YGMeasureModeExactly && ABI30_0_0YGFloatsEqual(size, lastComputedSize);
}

static inline bool ABI30_0_0YGMeasureModeOldSizeIsUnspecifiedAndStillFits(ABI30_0_0YGMeasureMode sizeMode,
                                                                 float size,
                                                                 ABI30_0_0YGMeasureMode lastSizeMode,
                                                                 float lastComputedSize) {
  return sizeMode == ABI30_0_0YGMeasureModeAtMost && lastSizeMode == ABI30_0_0YGMeasureModeUndefined &&
         (size >= lastComputedSize || ABI30_0_0YGFloatsEqual(size, lastComputedSize));
}

static inline bool ABI30_0_0YGMeasureModeNewMeasureSizeIsStricterAndStillValid(ABI30_0_0YGMeasureMode sizeMode,
                                                                      float size,
                                                                      ABI30_0_0YGMeasureMode lastSizeMode,
                                                                      float lastSize,
                                                                      float lastComputedSize) {
  return lastSizeMode == ABI30_0_0YGMeasureModeAtMost &&
      sizeMode == ABI30_0_0YGMeasureModeAtMost && !ABI30_0_0YGFloatIsUndefined(lastSize) &&
      !ABI30_0_0YGFloatIsUndefined(size) && !ABI30_0_0YGFloatIsUndefined(lastComputedSize) &&
      lastSize > size &&
      (lastComputedSize <= size || ABI30_0_0YGFloatsEqual(size, lastComputedSize));
}

float ABI30_0_0YGRoundValueToPixelGrid(const float value,
                              const float pointScaleFactor,
                              const bool forceCeil,
                              const bool forceFloor) {
  float scaledValue = value * pointScaleFactor;
  float fractial = fmodf(scaledValue, 1.0);
  if (ABI30_0_0YGFloatsEqual(fractial, 0)) {
    // First we check if the value is already rounded
    scaledValue = scaledValue - fractial;
  } else if (ABI30_0_0YGFloatsEqual(fractial, 1.0)) {
    scaledValue = scaledValue - fractial + 1.0;
  } else if (forceCeil) {
    // Next we check if we need to use forced rounding
    scaledValue = scaledValue - fractial + 1.0f;
  } else if (forceFloor) {
    scaledValue = scaledValue - fractial;
  } else {
    // Finally we just round the value
    scaledValue = scaledValue - fractial +
        (!ABI30_0_0YGFloatIsUndefined(fractial) &&
                 (fractial > 0.5f || ABI30_0_0YGFloatsEqual(fractial, 0.5f))
             ? 1.0f
             : 0.0f);
  }
  return (ABI30_0_0YGFloatIsUndefined(scaledValue) ||
          ABI30_0_0YGFloatIsUndefined(pointScaleFactor))
      ? ABI30_0_0YGUndefined
      : scaledValue / pointScaleFactor;
}

bool ABI30_0_0YGNodeCanUseCachedMeasurement(const ABI30_0_0YGMeasureMode widthMode,
                                   const float width,
                                   const ABI30_0_0YGMeasureMode heightMode,
                                   const float height,
                                   const ABI30_0_0YGMeasureMode lastWidthMode,
                                   const float lastWidth,
                                   const ABI30_0_0YGMeasureMode lastHeightMode,
                                   const float lastHeight,
                                   const float lastComputedWidth,
                                   const float lastComputedHeight,
                                   const float marginRow,
                                   const float marginColumn,
                                   const ABI30_0_0YGConfigRef config) {
  if ((!ABI30_0_0YGFloatIsUndefined(lastComputedHeight) && lastComputedHeight < 0) ||
      (!ABI30_0_0YGFloatIsUndefined(lastComputedWidth) && lastComputedWidth < 0)) {
    return false;
  }
  bool useRoundedComparison =
      config != nullptr && config->pointScaleFactor != 0;
  const float effectiveWidth =
      useRoundedComparison ? ABI30_0_0YGRoundValueToPixelGrid(width, config->pointScaleFactor, false, false)
                           : width;
  const float effectiveHeight =
      useRoundedComparison ? ABI30_0_0YGRoundValueToPixelGrid(height, config->pointScaleFactor, false, false)
                           : height;
  const float effectiveLastWidth =
      useRoundedComparison
          ? ABI30_0_0YGRoundValueToPixelGrid(lastWidth, config->pointScaleFactor, false, false)
          : lastWidth;
  const float effectiveLastHeight =
      useRoundedComparison
          ? ABI30_0_0YGRoundValueToPixelGrid(lastHeight, config->pointScaleFactor, false, false)
          : lastHeight;

  const bool hasSameWidthSpec =
      lastWidthMode == widthMode && ABI30_0_0YGFloatsEqual(effectiveLastWidth, effectiveWidth);
  const bool hasSameHeightSpec =
      lastHeightMode == heightMode && ABI30_0_0YGFloatsEqual(effectiveLastHeight, effectiveHeight);

  const bool widthIsCompatible =
      hasSameWidthSpec || ABI30_0_0YGMeasureModeSizeIsExactAndMatchesOldMeasuredSize(widthMode,
                                                                            width - marginRow,
                                                                            lastComputedWidth) ||
      ABI30_0_0YGMeasureModeOldSizeIsUnspecifiedAndStillFits(widthMode,
                                                    width - marginRow,
                                                    lastWidthMode,
                                                    lastComputedWidth) ||
      ABI30_0_0YGMeasureModeNewMeasureSizeIsStricterAndStillValid(
          widthMode, width - marginRow, lastWidthMode, lastWidth, lastComputedWidth);

  const bool heightIsCompatible =
      hasSameHeightSpec || ABI30_0_0YGMeasureModeSizeIsExactAndMatchesOldMeasuredSize(heightMode,
                                                                             height - marginColumn,
                                                                             lastComputedHeight) ||
      ABI30_0_0YGMeasureModeOldSizeIsUnspecifiedAndStillFits(heightMode,
                                                    height - marginColumn,
                                                    lastHeightMode,
                                                    lastComputedHeight) ||
      ABI30_0_0YGMeasureModeNewMeasureSizeIsStricterAndStillValid(
          heightMode, height - marginColumn, lastHeightMode, lastHeight, lastComputedHeight);

  return widthIsCompatible && heightIsCompatible;
}

//
// This is a wrapper around the ABI30_0_0YGNodelayoutImpl function. It determines
// whether the layout request is redundant and can be skipped.
//
// Parameters:
//  Input parameters are the same as ABI30_0_0YGNodelayoutImpl (see above)
//  Return parameter is true if layout was performed, false if skipped
//
bool ABI30_0_0YGLayoutNodeInternal(const ABI30_0_0YGNodeRef node,
                          const float availableWidth,
                          const float availableHeight,
                          const ABI30_0_0YGDirection parentDirection,
                          const ABI30_0_0YGMeasureMode widthMeasureMode,
                          const ABI30_0_0YGMeasureMode heightMeasureMode,
                          const float parentWidth,
                          const float parentHeight,
                          const bool performLayout,
                          const char *reason,
                          const ABI30_0_0YGConfigRef config) {
  ABI30_0_0YGLayout* layout = &node->getLayout();

  gDepth++;

  const bool needToVisitNode =
      (node->isDirty() && layout->generationCount != gCurrentGenerationCount) ||
      layout->lastParentDirection != parentDirection;

  if (needToVisitNode) {
    // Invalidate the cached results.
    layout->nextCachedMeasurementsIndex = 0;
    layout->cachedLayout.widthMeasureMode = (ABI30_0_0YGMeasureMode) -1;
    layout->cachedLayout.heightMeasureMode = (ABI30_0_0YGMeasureMode) -1;
    layout->cachedLayout.computedWidth = -1;
    layout->cachedLayout.computedHeight = -1;
  }

  ABI30_0_0YGCachedMeasurement* cachedResults = nullptr;

  // Determine whether the results are already cached. We maintain a separate
  // cache for layouts and measurements. A layout operation modifies the
  // positions
  // and dimensions for nodes in the subtree. The algorithm assumes that each
  // node
  // gets layed out a maximum of one time per tree layout, but multiple
  // measurements
  // may be required to resolve all of the flex dimensions.
  // We handle nodes with measure functions specially here because they are the
  // most
  // expensive to measure, so it's worth avoiding redundant measurements if at
  // all possible.
  if (node->getMeasure() != nullptr) {
    const float marginAxisRow =
        node->getMarginForAxis(ABI30_0_0YGFlexDirectionRow, parentWidth);
    const float marginAxisColumn =
        node->getMarginForAxis(ABI30_0_0YGFlexDirectionColumn, parentWidth);

    // First, try to use the layout cache.
    if (ABI30_0_0YGNodeCanUseCachedMeasurement(widthMeasureMode,
                                      availableWidth,
                                      heightMeasureMode,
                                      availableHeight,
                                      layout->cachedLayout.widthMeasureMode,
                                      layout->cachedLayout.availableWidth,
                                      layout->cachedLayout.heightMeasureMode,
                                      layout->cachedLayout.availableHeight,
                                      layout->cachedLayout.computedWidth,
                                      layout->cachedLayout.computedHeight,
                                      marginAxisRow,
                                      marginAxisColumn,
                                      config)) {
      cachedResults = &layout->cachedLayout;
    } else {
      // Try to use the measurement cache.
      for (uint32_t i = 0; i < layout->nextCachedMeasurementsIndex; i++) {
        if (ABI30_0_0YGNodeCanUseCachedMeasurement(widthMeasureMode,
                                          availableWidth,
                                          heightMeasureMode,
                                          availableHeight,
                                          layout->cachedMeasurements[i].widthMeasureMode,
                                          layout->cachedMeasurements[i].availableWidth,
                                          layout->cachedMeasurements[i].heightMeasureMode,
                                          layout->cachedMeasurements[i].availableHeight,
                                          layout->cachedMeasurements[i].computedWidth,
                                          layout->cachedMeasurements[i].computedHeight,
                                          marginAxisRow,
                                          marginAxisColumn,
                                          config)) {
          cachedResults = &layout->cachedMeasurements[i];
          break;
        }
      }
    }
  } else if (performLayout) {
    if (ABI30_0_0YGFloatsEqual(layout->cachedLayout.availableWidth, availableWidth) &&
        ABI30_0_0YGFloatsEqual(layout->cachedLayout.availableHeight, availableHeight) &&
        layout->cachedLayout.widthMeasureMode == widthMeasureMode &&
        layout->cachedLayout.heightMeasureMode == heightMeasureMode) {
      cachedResults = &layout->cachedLayout;
    }
  } else {
    for (uint32_t i = 0; i < layout->nextCachedMeasurementsIndex; i++) {
      if (ABI30_0_0YGFloatsEqual(layout->cachedMeasurements[i].availableWidth, availableWidth) &&
          ABI30_0_0YGFloatsEqual(layout->cachedMeasurements[i].availableHeight, availableHeight) &&
          layout->cachedMeasurements[i].widthMeasureMode == widthMeasureMode &&
          layout->cachedMeasurements[i].heightMeasureMode == heightMeasureMode) {
        cachedResults = &layout->cachedMeasurements[i];
        break;
      }
    }
  }

  if (!needToVisitNode && cachedResults != nullptr) {
    layout->measuredDimensions[ABI30_0_0YGDimensionWidth] = cachedResults->computedWidth;
    layout->measuredDimensions[ABI30_0_0YGDimensionHeight] = cachedResults->computedHeight;

    if (gPrintChanges && gPrintSkips) {
      ABI30_0_0YGLog(node, ABI30_0_0YGLogLevelVerbose, "%s%d.{[skipped] ", ABI30_0_0YGSpacer(gDepth), gDepth);
      if (node->getPrintFunc() != nullptr) {
        node->getPrintFunc()(node);
      }
      ABI30_0_0YGLog(
          node,
          ABI30_0_0YGLogLevelVerbose,
          "wm: %s, hm: %s, aw: %f ah: %f => d: (%f, %f) %s\n",
          ABI30_0_0YGMeasureModeName(widthMeasureMode, performLayout),
          ABI30_0_0YGMeasureModeName(heightMeasureMode, performLayout),
          availableWidth,
          availableHeight,
          cachedResults->computedWidth,
          cachedResults->computedHeight,
          reason);
    }
  } else {
    if (gPrintChanges) {
      ABI30_0_0YGLog(
          node,
          ABI30_0_0YGLogLevelVerbose,
          "%s%d.{%s",
          ABI30_0_0YGSpacer(gDepth),
          gDepth,
          needToVisitNode ? "*" : "");
      if (node->getPrintFunc() != nullptr) {
        node->getPrintFunc()(node);
      }
      ABI30_0_0YGLog(
          node,
          ABI30_0_0YGLogLevelVerbose,
          "wm: %s, hm: %s, aw: %f ah: %f %s\n",
          ABI30_0_0YGMeasureModeName(widthMeasureMode, performLayout),
          ABI30_0_0YGMeasureModeName(heightMeasureMode, performLayout),
          availableWidth,
          availableHeight,
          reason);
    }

    ABI30_0_0YGNodelayoutImpl(node,
                     availableWidth,
                     availableHeight,
                     parentDirection,
                     widthMeasureMode,
                     heightMeasureMode,
                     parentWidth,
                     parentHeight,
                     performLayout,
                     config);

    if (gPrintChanges) {
      ABI30_0_0YGLog(
          node,
          ABI30_0_0YGLogLevelVerbose,
          "%s%d.}%s",
          ABI30_0_0YGSpacer(gDepth),
          gDepth,
          needToVisitNode ? "*" : "");
      if (node->getPrintFunc() != nullptr) {
        node->getPrintFunc()(node);
      }
      ABI30_0_0YGLog(
          node,
          ABI30_0_0YGLogLevelVerbose,
          "wm: %s, hm: %s, d: (%f, %f) %s\n",
          ABI30_0_0YGMeasureModeName(widthMeasureMode, performLayout),
          ABI30_0_0YGMeasureModeName(heightMeasureMode, performLayout),
          layout->measuredDimensions[ABI30_0_0YGDimensionWidth],
          layout->measuredDimensions[ABI30_0_0YGDimensionHeight],
          reason);
    }

    layout->lastParentDirection = parentDirection;

    if (cachedResults == nullptr) {
      if (layout->nextCachedMeasurementsIndex == ABI30_0_0YG_MAX_CACHED_RESULT_COUNT) {
        if (gPrintChanges) {
          ABI30_0_0YGLog(node, ABI30_0_0YGLogLevelVerbose, "Out of cache entries!\n");
        }
        layout->nextCachedMeasurementsIndex = 0;
      }

      ABI30_0_0YGCachedMeasurement *newCacheEntry;
      if (performLayout) {
        // Use the single layout cache entry.
        newCacheEntry = &layout->cachedLayout;
      } else {
        // Allocate a new measurement cache entry.
        newCacheEntry = &layout->cachedMeasurements[layout->nextCachedMeasurementsIndex];
        layout->nextCachedMeasurementsIndex++;
      }

      newCacheEntry->availableWidth = availableWidth;
      newCacheEntry->availableHeight = availableHeight;
      newCacheEntry->widthMeasureMode = widthMeasureMode;
      newCacheEntry->heightMeasureMode = heightMeasureMode;
      newCacheEntry->computedWidth = layout->measuredDimensions[ABI30_0_0YGDimensionWidth];
      newCacheEntry->computedHeight = layout->measuredDimensions[ABI30_0_0YGDimensionHeight];
    }
  }

  if (performLayout) {
    node->setLayoutDimension(
        node->getLayout().measuredDimensions[ABI30_0_0YGDimensionWidth],
        ABI30_0_0YGDimensionWidth);
    node->setLayoutDimension(
        node->getLayout().measuredDimensions[ABI30_0_0YGDimensionHeight],
        ABI30_0_0YGDimensionHeight);

    node->setHasNewLayout(true);
    node->setDirty(false);
  }

  gDepth--;
  layout->generationCount = gCurrentGenerationCount;
  return (needToVisitNode || cachedResults == nullptr);
}

void ABI30_0_0YGConfigSetPointScaleFactor(const ABI30_0_0YGConfigRef config, const float pixelsInPoint) {
  ABI30_0_0YGAssertWithConfig(config, pixelsInPoint >= 0.0f, "Scale factor should not be less than zero");

  // We store points for Pixel as we will use it for rounding
  if (pixelsInPoint == 0.0f) {
    // Zero is used to skip rounding
    config->pointScaleFactor = 0.0f;
  } else {
    config->pointScaleFactor = pixelsInPoint;
  }
}

static void ABI30_0_0YGRoundToPixelGrid(const ABI30_0_0YGNodeRef node,
                               const float pointScaleFactor,
                               const float absoluteLeft,
                               const float absoluteTop) {
  if (pointScaleFactor == 0.0f) {
    return;
  }

  const float nodeLeft = node->getLayout().position[ABI30_0_0YGEdgeLeft];
  const float nodeTop = node->getLayout().position[ABI30_0_0YGEdgeTop];

  const float nodeWidth = node->getLayout().dimensions[ABI30_0_0YGDimensionWidth];
  const float nodeHeight = node->getLayout().dimensions[ABI30_0_0YGDimensionHeight];

  const float absoluteNodeLeft = absoluteLeft + nodeLeft;
  const float absoluteNodeTop = absoluteTop + nodeTop;

  const float absoluteNodeRight = absoluteNodeLeft + nodeWidth;
  const float absoluteNodeBottom = absoluteNodeTop + nodeHeight;

  // If a node has a custom measure function we never want to round down its size as this could
  // lead to unwanted text truncation.
  const bool textRounding = node->getNodeType() == ABI30_0_0YGNodeTypeText;

  node->setLayoutPosition(
      ABI30_0_0YGRoundValueToPixelGrid(nodeLeft, pointScaleFactor, false, textRounding),
      ABI30_0_0YGEdgeLeft);

  node->setLayoutPosition(
      ABI30_0_0YGRoundValueToPixelGrid(nodeTop, pointScaleFactor, false, textRounding),
      ABI30_0_0YGEdgeTop);

  // We multiply dimension by scale factor and if the result is close to the whole number, we don't
  // have any fraction
  // To verify if the result is close to whole number we want to check both floor and ceil numbers
  const bool hasFractionalWidth = !ABI30_0_0YGFloatsEqual(fmodf(nodeWidth * pointScaleFactor, 1.0), 0) &&
                                  !ABI30_0_0YGFloatsEqual(fmodf(nodeWidth * pointScaleFactor, 1.0), 1.0);
  const bool hasFractionalHeight = !ABI30_0_0YGFloatsEqual(fmodf(nodeHeight * pointScaleFactor, 1.0), 0) &&
                                   !ABI30_0_0YGFloatsEqual(fmodf(nodeHeight * pointScaleFactor, 1.0), 1.0);

  node->setLayoutDimension(
      ABI30_0_0YGRoundValueToPixelGrid(
          absoluteNodeRight,
          pointScaleFactor,
          (textRounding && hasFractionalWidth),
          (textRounding && !hasFractionalWidth)) -
          ABI30_0_0YGRoundValueToPixelGrid(
              absoluteNodeLeft, pointScaleFactor, false, textRounding),
      ABI30_0_0YGDimensionWidth);

  node->setLayoutDimension(
      ABI30_0_0YGRoundValueToPixelGrid(
          absoluteNodeBottom,
          pointScaleFactor,
          (textRounding && hasFractionalHeight),
          (textRounding && !hasFractionalHeight)) -
          ABI30_0_0YGRoundValueToPixelGrid(
              absoluteNodeTop, pointScaleFactor, false, textRounding),
      ABI30_0_0YGDimensionHeight);

  const uint32_t childCount = ABI30_0_0YGNodeGetChildCount(node);
  for (uint32_t i = 0; i < childCount; i++) {
    ABI30_0_0YGRoundToPixelGrid(
        ABI30_0_0YGNodeGetChild(node, i),
        pointScaleFactor,
        absoluteNodeLeft,
        absoluteNodeTop);
  }
}

void ABI30_0_0YGNodeCalculateLayout(
    const ABI30_0_0YGNodeRef node,
    const float parentWidth,
    const float parentHeight,
    const ABI30_0_0YGDirection parentDirection) {
  // Increment the generation count. This will force the recursive routine to
  // visit
  // all dirty nodes at least once. Subsequent visits will be skipped if the
  // input
  // parameters don't change.
  gCurrentGenerationCount++;
  node->resolveDimension();
  float width = ABI30_0_0YGUndefined;
  ABI30_0_0YGMeasureMode widthMeasureMode = ABI30_0_0YGMeasureModeUndefined;
  if (ABI30_0_0YGNodeIsStyleDimDefined(node, ABI30_0_0YGFlexDirectionRow, parentWidth)) {
    width =
        ABI30_0_0YGResolveValue(
            node->getResolvedDimension(dim[ABI30_0_0YGFlexDirectionRow]), parentWidth) +
        node->getMarginForAxis(ABI30_0_0YGFlexDirectionRow, parentWidth);
    widthMeasureMode = ABI30_0_0YGMeasureModeExactly;
  } else if (!ABI30_0_0YGFloatIsUndefined(ABI30_0_0YGResolveValue(
                 node->getStyle().maxDimensions[ABI30_0_0YGDimensionWidth],
                 parentWidth))) {
    width = ABI30_0_0YGResolveValue(
        node->getStyle().maxDimensions[ABI30_0_0YGDimensionWidth], parentWidth);
    widthMeasureMode = ABI30_0_0YGMeasureModeAtMost;
  } else {
    width = parentWidth;
    widthMeasureMode = ABI30_0_0YGFloatIsUndefined(width) ? ABI30_0_0YGMeasureModeUndefined
                                                 : ABI30_0_0YGMeasureModeExactly;
  }

  float height = ABI30_0_0YGUndefined;
  ABI30_0_0YGMeasureMode heightMeasureMode = ABI30_0_0YGMeasureModeUndefined;
  if (ABI30_0_0YGNodeIsStyleDimDefined(node, ABI30_0_0YGFlexDirectionColumn, parentHeight)) {
    height = ABI30_0_0YGResolveValue(
                 node->getResolvedDimension(dim[ABI30_0_0YGFlexDirectionColumn]),
                 parentHeight) +
        node->getMarginForAxis(ABI30_0_0YGFlexDirectionColumn, parentWidth);
    heightMeasureMode = ABI30_0_0YGMeasureModeExactly;
  } else if (!ABI30_0_0YGFloatIsUndefined(ABI30_0_0YGResolveValue(
                 node->getStyle().maxDimensions[ABI30_0_0YGDimensionHeight],
                 parentHeight))) {
    height = ABI30_0_0YGResolveValue(
        node->getStyle().maxDimensions[ABI30_0_0YGDimensionHeight], parentHeight);
    heightMeasureMode = ABI30_0_0YGMeasureModeAtMost;
  } else {
    height = parentHeight;
    heightMeasureMode = ABI30_0_0YGFloatIsUndefined(height) ? ABI30_0_0YGMeasureModeUndefined
                                                   : ABI30_0_0YGMeasureModeExactly;
  }
  if (ABI30_0_0YGLayoutNodeInternal(
          node,
          width,
          height,
          parentDirection,
          widthMeasureMode,
          heightMeasureMode,
          parentWidth,
          parentHeight,
          true,
          "initial",
          node->getConfig())) {
    node->setPosition(
        node->getLayout().direction, parentWidth, parentHeight, parentWidth);
    ABI30_0_0YGRoundToPixelGrid(node, node->getConfig()->pointScaleFactor, 0.0f, 0.0f);

    if (gPrintTree) {
      ABI30_0_0YGNodePrint(
          node,
          (ABI30_0_0YGPrintOptions)(
              ABI30_0_0YGPrintOptionsLayout | ABI30_0_0YGPrintOptionsChildren |
              ABI30_0_0YGPrintOptionsStyle));
    }
  }

  // We want to get rid off `useLegacyStretchBehaviour` from ABI30_0_0YGConfig. But we
  // aren't sure whether client's of yoga have gotten rid off this flag or not.
  // So logging this in ABI30_0_0YGLayout would help to find out the call sites depending
  // on this flag. This check would be removed once we are sure no one is
  // dependent on this flag anymore. The flag
  // `shouldDiffLayoutWithoutLegacyStretchBehaviour` in ABI30_0_0YGConfig will help to
  // run experiments.
  if (node->getConfig()->shouldDiffLayoutWithoutLegacyStretchBehaviour &&
      node->didUseLegacyFlag()) {
    const ABI30_0_0YGNodeRef originalNode = ABI30_0_0YGNodeDeepClone(node);
    originalNode->resolveDimension();
    // Recursively mark nodes as dirty
    originalNode->markDirtyAndPropogateDownwards();
    gCurrentGenerationCount++;
    // Rerun the layout, and calculate the diff
    originalNode->setAndPropogateUseLegacyFlag(false);
    if (ABI30_0_0YGLayoutNodeInternal(
            originalNode,
            width,
            height,
            parentDirection,
            widthMeasureMode,
            heightMeasureMode,
            parentWidth,
            parentHeight,
            true,
            "initial",
            originalNode->getConfig())) {
      originalNode->setPosition(
          originalNode->getLayout().direction,
          parentWidth,
          parentHeight,
          parentWidth);
      ABI30_0_0YGRoundToPixelGrid(
          originalNode,
          originalNode->getConfig()->pointScaleFactor,
          0.0f,
          0.0f);

      // Set whether the two layouts are different or not.
      node->setLayoutDoesLegacyFlagAffectsLayout(
          !originalNode->isLayoutTreeEqualToNode(*node));

      if (gPrintTree) {
        ABI30_0_0YGNodePrint(
            originalNode,
            (ABI30_0_0YGPrintOptions)(
                ABI30_0_0YGPrintOptionsLayout | ABI30_0_0YGPrintOptionsChildren |
                ABI30_0_0YGPrintOptionsStyle));
      }
    }
    ABI30_0_0YGConfigFreeRecursive(originalNode);
    ABI30_0_0YGNodeFreeRecursive(originalNode);
  }
}

void ABI30_0_0YGConfigSetLogger(const ABI30_0_0YGConfigRef config, ABI30_0_0YGLogger logger) {
  if (logger != nullptr) {
    config->logger = logger;
  } else {
#ifdef ANDROID
    config->logger = &ABI30_0_0YGAndroidLog;
#else
    config->logger = &ABI30_0_0YGDefaultLog;
#endif
  }
}

static void ABI30_0_0YGVLog(const ABI30_0_0YGConfigRef config,
                   const ABI30_0_0YGNodeRef node,
                   ABI30_0_0YGLogLevel level,
                   const char *format,
                   va_list args) {
  const ABI30_0_0YGConfigRef logConfig = config != nullptr ? config : &gABI30_0_0YGConfigDefaults;
  logConfig->logger(logConfig, node, level, format, args);

  if (level == ABI30_0_0YGLogLevelFatal) {
    abort();
  }
}

void ABI30_0_0YGLogWithConfig(const ABI30_0_0YGConfigRef config, ABI30_0_0YGLogLevel level, const char *format, ...) {
  va_list args;
  va_start(args, format);
  ABI30_0_0YGVLog(config, nullptr, level, format, args);
  va_end(args);
}

void ABI30_0_0YGLog(const ABI30_0_0YGNodeRef node, ABI30_0_0YGLogLevel level, const char *format, ...) {
  va_list args;
  va_start(args, format);
  ABI30_0_0YGVLog(
      node == nullptr ? nullptr : node->getConfig(), node, level, format, args);
  va_end(args);
}

void ABI30_0_0YGAssert(const bool condition, const char *message) {
  if (!condition) {
    ABI30_0_0YGLog(nullptr, ABI30_0_0YGLogLevelFatal, "%s\n", message);
  }
}

void ABI30_0_0YGAssertWithNode(const ABI30_0_0YGNodeRef node, const bool condition, const char *message) {
  if (!condition) {
    ABI30_0_0YGLog(node, ABI30_0_0YGLogLevelFatal, "%s\n", message);
  }
}

void ABI30_0_0YGAssertWithConfig(const ABI30_0_0YGConfigRef config, const bool condition, const char *message) {
  if (!condition) {
    ABI30_0_0YGLogWithConfig(config, ABI30_0_0YGLogLevelFatal, "%s\n", message);
  }
}

void ABI30_0_0YGConfigSetExperimentalFeatureEnabled(const ABI30_0_0YGConfigRef config,
                                           const ABI30_0_0YGExperimentalFeature feature,
                                           const bool enabled) {
  config->experimentalFeatures[feature] = enabled;
}

inline bool ABI30_0_0YGConfigIsExperimentalFeatureEnabled(const ABI30_0_0YGConfigRef config,
                                                 const ABI30_0_0YGExperimentalFeature feature) {
  return config->experimentalFeatures[feature];
}

void ABI30_0_0YGConfigSetUseWebDefaults(const ABI30_0_0YGConfigRef config, const bool enabled) {
  config->useWebDefaults = enabled;
}

void ABI30_0_0YGConfigSetUseLegacyStretchBehaviour(const ABI30_0_0YGConfigRef config,
                                          const bool useLegacyStretchBehaviour) {
  config->useLegacyStretchBehaviour = useLegacyStretchBehaviour;
}

bool ABI30_0_0YGConfigGetUseWebDefaults(const ABI30_0_0YGConfigRef config) {
  return config->useWebDefaults;
}

void ABI30_0_0YGConfigSetContext(const ABI30_0_0YGConfigRef config, void *context) {
  config->context = context;
}

void *ABI30_0_0YGConfigGetContext(const ABI30_0_0YGConfigRef config) {
  return config->context;
}

void ABI30_0_0YGConfigSetNodeClonedFunc(const ABI30_0_0YGConfigRef config, const ABI30_0_0YGNodeClonedFunc callback) {
  config->cloneNodeCallback = callback;
}
