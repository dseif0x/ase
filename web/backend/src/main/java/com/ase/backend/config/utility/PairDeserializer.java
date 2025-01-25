/*
 * Copyright (c) 2025 Project Contributors
 *
 * This source code is licensed under the MIT License found in the LICENSE file in the root directory
 * of this source tree or at https://opensource.org/licenses/MIT.
 */
package com.ase.backend.config.utility;

import com.ase.utility.tcp.TcpGeneral;
import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.databind.DeserializationContext;
import com.fasterxml.jackson.databind.JsonDeserializer;
import com.fasterxml.jackson.databind.JsonNode;
import org.apache.commons.lang3.tuple.ImmutablePair;
import org.apache.commons.lang3.tuple.Pair;

import java.io.IOException;

public class PairDeserializer extends JsonDeserializer<Pair> {
    @Override
    public Pair<?, ?> deserialize(JsonParser p, DeserializationContext ctxt) throws IOException {
        JsonNode node = p.getCodec().readTree(p);

        JsonNode leftNode = node.get("left");
        JsonNode rightNode = node.get("right");

        TcpGeneral left = null;
        TcpGeneral right = null;

        if (leftNode != null) {
            left = p.getCodec().treeToValue(leftNode, TcpGeneral.class);
        }

        if (rightNode != null) {
            right = p.getCodec().treeToValue(rightNode, TcpGeneral.class);
        }

        return new ImmutablePair<>(left, right);
    }
}