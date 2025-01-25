/*
 * Copyright (c) 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */
package com.ase.utility.tcp;

public class TypeDefinition {
    private final boolean isArray;
    private final boolean isPair;
    private final boolean isComplex;
    private final boolean isKeyMap;
    private final boolean isOnlyType;
    private final SimpleType simpleType;

    public TypeDefinition(boolean isArray, boolean isPair, boolean isComplex, boolean isKeyMap, boolean isOnlyType, SimpleType simpleType) {
        this.isArray = isArray;
        this.isPair = isPair;
        this.isComplex = isComplex;
        this.isKeyMap = isKeyMap;
        this.isOnlyType = isOnlyType;
        this.simpleType = simpleType;
    }

    public boolean isArray() {
        return isArray;
    }

    public boolean isKeyMap() {
        return isKeyMap;
    }

    public boolean isPair() {
        return isPair;
    }

    public boolean isComplex() {
        return isComplex;
    }

    public boolean isOnlyType() {
        return isOnlyType;
    }

    public SimpleType getSimpleType() {
        return simpleType;
    }
}
