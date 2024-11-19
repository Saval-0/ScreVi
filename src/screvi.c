#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define NOB_IMPLEMENTATION
#include "../nob.h"

#define MAX_FILE_SIZE 10240
#define NUM_SCREENS 3

// Function to get file size
long get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) != 0) {
        fprintf(stderr, "Error getting file size for %s\n", filename);
        return -1;
    }
    return st.st_size;
}

// Function to compress image if it exceeds size limit
int compress_image(const char *filename) {
    char cmd[512];
    char temp_filename[256];
    
    // Create temporary filename
    snprintf(temp_filename, sizeof(temp_filename), "temp_%s", filename);
    
    // Using ImageMagick's convert command
    snprintf(cmd, sizeof(cmd),
        "magick \"%s\" png32:\"%s\"",
        filename, temp_filename);
    
    printf("Compressing image %s...\n", filename);
    
    // Execute compression command
    if (system(cmd) != 0) {
        fprintf(stderr, "Error compressing image %s\n", filename);
        remove(temp_filename);
        return -1;
    }
    
    // Replace original file with compressed version
    if (rename(temp_filename, filename) != 0) {
        fprintf(stderr, "Error replacing original file with compressed version\n");
        remove(temp_filename);
        return -1;
    }
    
    printf("Image compressed successfully\n");
    return 0;
}

void seconds_to_hhmmss(double total_seconds, char *output) {
    int hours = (int)(total_seconds / 3600);
    int minutes = (int)((total_seconds - hours * 3600) / 60);
    int seconds = (int)(total_seconds - hours * 3600 - minutes * 60);
    
    sprintf(output, "%02d:%02d:%02d", hours, minutes, seconds);
}

double get_video_length(const char *filename) {
    char cmd[256];
    char buffer[128];
    double duration = 0.0;
    
    snprintf(cmd, sizeof(cmd),
        "ffprobe -v error -show_entries format=duration "
        "-of default=noprint_wrappers=1:nokey=1 \"%s\" 2>/dev/null",
        filename);
    
    FILE *pipe = popen(cmd, "r");
    if (!pipe) {
        fprintf(stderr, "Error executing ffprobe command\n");
        return -1;
    }
    
    if (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        duration = atof(buffer);
    }
    
    pclose(pipe);
    return duration;
}

int generate_screenshot(const char *input_file, double timestamp, int screenshot_number) {
    char cmd[512];
    char timestamp_str[9];
    char output_filename[32];
    
    // Convert timestamp to HH:MM:SS format
    seconds_to_hhmmss(timestamp, timestamp_str);
    
    // Create output filename
    snprintf(output_filename, sizeof(output_filename), "res/output_%d.png", screenshot_number);
    
    // Construct ffmpeg command
    snprintf(cmd, sizeof(cmd),
        "ffmpeg -ss %s -y -i \"%s\" -frames:v 1 %s >/dev/null 2>&1",
        timestamp_str, input_file, output_filename);
    
    printf("Executing command: ffmpeg -ss %s -y -i \"%s\" -frames:v 1 %s\n",
           timestamp_str, input_file, output_filename);
    
    // Execute command
    int result = system(cmd);
    if (result != 0) {
        fprintf(stderr, "Error generating screenshot %d at %s\n", 
                screenshot_number, timestamp_str);
        return -1;
    }
    
    printf("Generated screenshot %d at %s (%.2f seconds)\n", 
           screenshot_number, timestamp_str, timestamp);
           
    // Check file size and compress if necessary
    long file_size = get_file_size(output_filename);
    if (file_size > MAX_FILE_SIZE) {
        printf("Screenshot %s exceeds %d MB (current size: %.2f MB). Compressing...\n", 
               output_filename, MAX_FILE_SIZE/1024, file_size / 1048576.0);
        if (compress_image(output_filename) < 0) {
            return -1;
        }
        
        // Verify new file size
        file_size = get_file_size(output_filename);
        printf("New file size: %.2f MB\n", file_size / 1048576.0);
    }
    
    return 0;
}

int main(int argc, char **argv) 
{
    const char *program_name = nob_shift(argv, argc);

    const char *v_path;
    if (argc > 0) {
        v_path = nob_shift(argv, argc);
    } else {
        nob_log(NOB_ERROR, "Provide path to video file.");
        return 1;
    }

    double duration = get_video_length(v_path);
    if (duration < 0) {
        fprintf(stderr, "Error getting video duration\n");
        return 1;
    }
    
    char formatted_time[9];
    seconds_to_hhmmss(duration, formatted_time);
    printf("\nVideo Information:\n");
    printf("================\n");
    printf("Video path: %s\n", v_path);
    printf("Duration: %s (%.2f seconds)\n\n", formatted_time, duration);

    free(v_path);
    
    // Calculate screenshot timestamps
    double screenshot_times[] = {
        duration * 0.10,  // 10% of video
        duration * 0.25,  // 25% of video
        duration * 0.40   // 40% of video
    };
    
    printf("Screenshot Timestamps:\n");
    printf("====================\n");
    char time_str[9];
    for (int i = 0; i < NUM_SCREENS; i++) {
        seconds_to_hhmmss(screenshot_times[i], time_str);
        printf("Screenshot %d: %s (%.2f seconds) - %.1f%% of video\n", 
               i + 1, time_str, screenshot_times[i], (i == 0 ? 10.0 : (i == 1 ? 25.0 : 40.0)));
    }
    printf("\n");
    
    printf("Generating Screenshots:\n");
    printf("=====================\n");
    for (int i = 0; i < NUM_SCREENS; i++) {
        printf("\nProcessing screenshot %d:\n", i + 1);
        printf("------------------------\n");
        if (generate_screenshot(v_path, screenshot_times[i], i + 1) < 0) {
            fprintf(stderr, "Failed to generate all screenshots\n");
            return 1;
        }
        printf("------------------------\n");
    }
    
    printf("\nSuccessfully generated all screenshots\n");
    return 0;
}