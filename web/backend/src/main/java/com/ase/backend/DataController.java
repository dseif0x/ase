package com.ase.backend;

import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.HashMap;
import java.util.Map;

@RestController
@RequestMapping("/api")
public class DataController {

    @GetMapping("/data")
    public Map<String, Object> getData() {
        Map<String, Object> data = new HashMap<>();
        data.put("message", "Hello World!");
        data.put("status", "success");
        data.put("timestamp", System.currentTimeMillis());
        return data;
    }
}
