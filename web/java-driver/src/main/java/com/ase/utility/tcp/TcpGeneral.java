package com.ase.utility.tcp;

import org.apache.commons.lang3.tuple.Pair;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Objects;

/**
 * Represents a general TCP data type which can be a simple type, array, pair, or key-map.
 */
public class TcpGeneral {

    private TcpGeneralType type;
    private SimpleType simpleType; // Applicable if type == SIMPLE
    private Object value; // The actual value for SIMPLE type
    private List<TcpGeneral> array = new ArrayList<>(); // Applicable if type == ARRAY
    private Pair<TcpGeneral, TcpGeneral> pair = Pair.of(null, null); // Applicable if type == PAIR
    private HashMap<String, TcpGeneral> keyMap = new HashMap<>(); // Applicable if type == KEY_MAP

    public TcpGeneral() {}

    /**
     * Private constructor to enforce the use of factory methods.
     */
    private TcpGeneral(TcpGeneralType type, SimpleType simpleType, Object value,
                       List<TcpGeneral> array, Pair<TcpGeneral, TcpGeneral> pair, HashMap<String, TcpGeneral> keyMap) {
        this.type = type;
        this.simpleType = simpleType;
        this.value = value;
        this.array = array;
        this.pair = pair;
        this.keyMap = keyMap;
    }

    // Factory methods for different types

    /**
     * Creates a TcpGeneral instance representing a simple type.
     *
     * @param simpleType The simple type.
     * @param value      The value of the simple type.
     * @return A TcpGeneral instance.
     */
    public static TcpGeneral ofSimple(SimpleType simpleType, Object value) {
        return new TcpGeneral(TcpGeneralType.SIMPLE, simpleType, value, null, null, null);
    }

    /**
     * Creates a TcpGeneral instance representing an array.
     *
     * @param array The list of TcpGeneral elements.
     * @return A TcpGeneral instance.
     */
    public static TcpGeneral ofArray(List<TcpGeneral> array) {
        return new TcpGeneral(TcpGeneralType.ARRAY, null, null, array, null, null);
    }

    /**
     * Creates a TcpGeneral instance representing a pair.
     *
     * @param first  The first element of the pair.
     * @param second The second element of the pair.
     * @return A TcpGeneral instance.
     */
    public static TcpGeneral ofPair(TcpGeneral first, TcpGeneral second) {
        return new TcpGeneral(TcpGeneralType.PAIR, null, null, null, Pair.of(first, second), null);
    }

    /**
     * Creates a TcpGeneral instance representing a key-map.
     *
     * @param keyMap The key-map with String keys and TcpGeneral values.
     * @return A TcpGeneral instance.
     */
    public static TcpGeneral ofKeyMap(HashMap<String, TcpGeneral> keyMap) {
        return new TcpGeneral(TcpGeneralType.KEY_MAP, null, null, null, null, keyMap);
    }

    // Getters

    public TcpGeneralType getType() {
        return type;
    }

    public SimpleType getSimpleType() {
        return simpleType;
    }

    public Object getValue() {
        return value;
    }

    public List<TcpGeneral> getArray() {
        return array;
    }

    public Pair<TcpGeneral, TcpGeneral> getPair() {
        return pair;
    }

    public HashMap<String, TcpGeneral> getKeyMap() {
        return keyMap;
    }

    public SimpleType getElementType() {
        if (type == TcpGeneralType.ARRAY && array != null && !array.isEmpty()) {
            return array.get(0).getSimpleType();
        } else if (type == TcpGeneralType.PAIR && pair != null && pair.getLeft() != null) {
            return pair.getLeft().getSimpleType();
        }
        return SimpleType.UNKNOWN;
    }

    public boolean isComplex() {
        switch (type) {
            case ARRAY:
                return array != null && (array.stream().anyMatch(TcpGeneral::_isComplex) ||
                    array.stream().map(TcpGeneral::getSimpleType).distinct().count() > 1);
            case PAIR:
                return pair != null && pair.getLeft() != null && pair.getLeft()._isComplex();
            case KEY_MAP:
                return keyMap != null;
            case SIMPLE:
            default:
                return false;
        }
    }

    private boolean _isComplex() {
        switch (type) {
            case ARRAY:
            case KEY_MAP:
            case PAIR:
                return true;
            case SIMPLE:
            default:
                return false;
        }
    }

    @Override
    public String toString() {
        switch (type) {
            case SIMPLE:
                return "TcpGeneral{" +
                    "type=SIMPLE(" + simpleType + ")" +
                    ", value=" + value +
                    '}';
            case ARRAY:
                return "TcpGeneral{" +
                    "type=ARRAY" +
                    ", array=" + array +
                    '}';
            case PAIR:
                return "TcpGeneral{" +
                    "type=PAIR" +
                    ", pair=" + pair +
                    '}';
            case KEY_MAP:
                return "TcpGeneral{" +
                    "type=KEY_MAP" +
                    ", keyMap=" + keyMap +
                    '}';
            default:
                return "TcpGeneral{Unknown Type}";
        }
    }

    // Override equals and hashCode for testing and consistency

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (!(o instanceof TcpGeneral)) return false;
        TcpGeneral that = (TcpGeneral) o;
        return type == that.type &&
            simpleType == that.simpleType &&
            Objects.equals(value, that.value) &&
            Objects.equals(array, that.array) &&
            Objects.equals(pair, that.pair) &&
            Objects.equals(keyMap, that.keyMap);
    }

    @Override
    public int hashCode() {
        return Objects.hash(type, simpleType, value, array, pair, keyMap);
    }


    // Setters
    public void setType(TcpGeneralType type) {
        this.type = type;
    }

    public void setSimpleType(SimpleType simpleType) {
        this.simpleType = simpleType;
    }

    public void setValue(Object value) {
        this.value = value;
    }

    public void setArray(List<TcpGeneral> array) {
        this.array = array;
    }

    public void setPair(Pair<TcpGeneral, TcpGeneral> pair) {
        this.pair = pair;
    }

    public void setKeyMap(HashMap<String, TcpGeneral> keyMap) {
        this.keyMap = keyMap;
    }
}
