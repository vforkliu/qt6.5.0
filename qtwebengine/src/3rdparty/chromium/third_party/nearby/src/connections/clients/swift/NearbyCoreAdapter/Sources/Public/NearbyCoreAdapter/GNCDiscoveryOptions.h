// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSInteger, GNCStrategy);

// TODO(b/239610253): Current sdk only exposes `strategy`, but we should add full support.
/**
 * A @c GNCDiscoveryOptions object represents the configuration for remote endpoint discovery.
 */
@interface GNCDiscoveryOptions : NSObject

/**
 * The connection strategy.
 */
@property(nonatomic, readonly) GNCStrategy strategy;

/**
 * @remark init is not an available initializer.
 */
- (nonnull instancetype)init NS_UNAVAILABLE;

/**
 * Creates a discovery options object.
 *
 * @param strategy The connection strategy.
 *
 * @return The initialized options object, or nil if an error occurs.
 */
- (nonnull instancetype)initWithStrategy:(GNCStrategy)strategy NS_DESIGNATED_INITIALIZER;

@end
