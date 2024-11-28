package com.ase.utility.tcp;
/**
 * Enumeration representing simple data types.
 * Extend this enumeration based on your protocol's supported types.
 */
public enum SimpleType {
    BOOLEAN(0x01),
    INT(0x02),
    STRING(0x03),
    DOUBLE(0x04),
    SEMAPHORE(0x05),
    REFERENCE(0x06);

    /**
     * The type ID.
     */
    private final int typeId;

    SimpleType(int typeId) {
        this.typeId = typeId;
    }

    /**
     * Retrieves the SimpleType based on the provided type ID.
     *
     * @param typeId The type ID to lookup.
     * @return The corresponding SimpleType.
     * @throws IllegalArgumentException if the type ID is unknown.
     */
    public static SimpleType fromTypeId(int typeId) {
        for (SimpleType type : values()) {
            if (type.typeId == typeId) {
                return type;
            }
        }
        throw new IllegalArgumentException("Unknown SimpleType ID: " + typeId);
    }
}