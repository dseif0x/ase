package com.ase.utility.tcp;

public class TypeDefinition {
    private final boolean isArray;
    private final boolean isPair;
    private final boolean isComplex;
    private final boolean isKey;
    private final boolean isOnlyType;
    private final SimpleType simpleType;

    public TypeDefinition(boolean isArray, boolean isPair, boolean isComplex, boolean isKey, boolean isOnlyType, SimpleType simpleType) {
        this.isArray = isArray;
        this.isPair = isPair;
        this.isComplex = isComplex;
        this.isKey = isKey;
        this.isOnlyType = isOnlyType;
        this.simpleType = simpleType;
    }

    public boolean isArray() {
        return isArray;
    }

    public boolean isKey() {
        return isKey;
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
