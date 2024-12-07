package com.ase.backend;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.HashMap;
import java.util.Map;

@RestController
@RequestMapping("/api")
public class DataController {

    private static final Logger logger = LoggerFactory.getLogger(DataController.class);

    @GetMapping("/data")
    public Map<String, Object> getData() {
        logger.info("/data");
        Map<String, Object> data = new HashMap<>();
        data.put("message", "Hello World!");
        data.put("status", "success");
        data.put("timestamp", System.currentTimeMillis());
        return data;
    }
}
