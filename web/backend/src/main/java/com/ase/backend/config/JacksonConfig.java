package com.ase.backend.config;

import com.ase.backend.config.utility.PairDeserializer;
import com.ase.backend.config.utility.PairSerializer;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.module.SimpleModule;
import org.apache.commons.lang3.tuple.Pair;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

@Configuration
public class JacksonConfig {

    @Bean
    public static ObjectMapper configureObjectMapper() {
        ObjectMapper objectMapper = new ObjectMapper();
        SimpleModule module = new SimpleModule();
        module.addSerializer(Pair.class, new PairSerializer());
        module.addDeserializer(Pair.class, new PairDeserializer());
        objectMapper.registerModule(module);
        return objectMapper;
    }
}
