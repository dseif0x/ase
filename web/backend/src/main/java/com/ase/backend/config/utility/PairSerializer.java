package com.ase.backend.config.utility;

import com.fasterxml.jackson.core.JsonGenerator;
import com.fasterxml.jackson.databind.JsonSerializer;
import com.fasterxml.jackson.databind.SerializerProvider;
import org.apache.commons.lang3.tuple.Pair;

import java.io.IOException;

public class PairSerializer extends JsonSerializer<Pair> {
    @Override
    public void serialize(Pair pair, JsonGenerator gen, SerializerProvider serializers) throws IOException {
        gen.writeStartObject();
        gen.writeObjectField("left", pair.getLeft());
        gen.writeObjectField("right", pair.getRight());
        gen.writeEndObject();
    }
}