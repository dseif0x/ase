package com.ase.backend.controller.dto;

import com.ase.utility.tcp.TcpGeneral;

public class SetValueRequest {
    private String key;
    private TcpGeneral value;
    private boolean force;

    public String getKey() {
        return key;
    }

    public void setKey(String key) {
        this.key = key;
    }

    public TcpGeneral getValue() {
        return value;
    }

    public void setValue(TcpGeneral value) {
        this.value = value;
    }

    public boolean isForce() {
        return force;
    }

    public void setForce(boolean force) {
        this.force = force;
    }
}
