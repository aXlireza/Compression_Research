#include <Arduino.h>
// The iamge should be compressed before entering the arduino duo to the memory constrains
// Function to perform RLE compression
String rleCompress(const uint8_t* data, size_t length) {
    String compressed = "";
    for (size_t i = 0; i < length; i++) {
        uint8_t count = 1;
        while (i + 1 < length && data[i] == data[i + 1]) {
            count++;
            i++;
        }
        compressed += String(data[i]) + ":" + String(count) + ",";
    }
    return compressed;
}

// Function to perform RLE decompression
void rleDecompress(const String& compressed, uint8_t* decompressed, size_t& decompressedLength) {
    decompressedLength = 0;
    int index = 0;
    int pos = 0;
    while ((index = compressed.indexOf(',', pos)) != -1) {
        int colonPos = compressed.indexOf(':', pos);
        uint8_t value = compressed.substring(pos, colonPos).toInt();
        uint8_t count = compressed.substring(colonPos + 1, index).toInt();
        for (uint8_t i = 0; i < count; i++) {
            decompressed[decompressedLength++] = value;
        }
        pos = index + 1;
    }
}

void setup() {
    // Example image data (must be very small due to Arduino memory constraints)
    const uint8_t imageData[] = {1, 1, 1, 2, 2, 3, 3, 3, 3};
    size_t length = sizeof(imageData) / sizeof(imageData[0]);

    // Compress the image data
    String compressed = rleCompress(imageData, length);
    Serial.begin(9600);
    Serial.println("Compressed Data: " + compressed);

    // Decompress the image data
    uint8_t decompressed[100]; // Adjust size based on expected output
    size_t decompressedLength;
    rleDecompress(compressed, decompressed, decompressedLength);

    // Print decompressed data
    Serial.print("Decompressed Data: ");
    for (size_t i = 0; i < decompressedLength; i++) {
        Serial.print(decompressed[i]);
        Serial.print(" ");
    }
}

void loop() {
    // Nothing to do here
}
