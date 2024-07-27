import streamlit as st
import cv2
import numpy as np
import urllib.request
import pandas as pd
import time
from ultralytics import YOLO
from gtts import gTTS
from playsound import playsound
import os
import tempfile
import requests

# Base URLs for the camera feed and boat control
base_url = 'http://192.168.69.198'
cam_hi_url = f'{base_url}/cam-hi.jpg'
ada_sampah_url = f'{base_url}/ada_sampah'

# Initialize a dataframe to store detection timestamps and counts
df = pd.DataFrame(columns=['Timestamp', 'Count'])

# Load YOLO model
model = YOLO('best.pt')  # Ensure 'best.pt' is the correct path to your YOLO model

# Streamlit app title and description
st.title('Garbage Detection and Collection System')
st.write('This application detects garbage and directs the boat to pick it up.')

# Placeholder for video feed
video_feed = st.empty()

# Placeholder for cropped images
cropped_images = st.empty()

# Function to update the plot
def update_plot():
    chart_data = df.set_index('Timestamp')
    st.bar_chart(chart_data)

# Detection function
def run_detection():
    global df
    cv2.namedWindow("detection", cv2.WINDOW_AUTOSIZE)
    last_minute = time.time()
    latest_objects = []
    
    cropped_imgs = []
    while True:
        current_time = time.time()
        if current_time - last_minute >= 60:  # Check if a minute has passed
            update_plot()
            last_minute = current_time

        try:
            # Initialize confidences list
            confidences = []

            # Get image from camera
            img_resp = urllib.request.urlopen(cam_hi_url)
            imgnp = np.array(bytearray(img_resp.read()), dtype=np.uint8)
            im = cv2.imdecode(imgnp, -1)

            # Detect objects
            results = model(im)
            detected_objects = []
            for result in results:
                for box in result.boxes:
                    x1, y1, x2, y2 = map(int, box.xyxy[0])
                    conf = box.conf[0]
                    label = model.names[int(box.cls[0])]
                    if label == "NotTrash":
                        label = "Trash"
                    
                    # Hanya memproses deteksi dengan nilai kepercayaan lebih dari 50%
                    if conf > 0.2:
                        detected_objects.append(label)
                        confidences.append(conf)

                    cv2.rectangle(im, (x1, y1), (x2, y2), (0, 255, 0), 2)
                    cv2.putText(im, f'{label} {conf:.2f}', (x1, y1 - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.9, (0, 255, 0), 2)

                    # Crop the detected object and store it
                    cropped_img = im[y1:y2, x1:x2]
                    cropped_imgs.append(cropped_img)

            # Update dataframe and plot every minute
            if current_time - last_minute >= 60:
                df = df.append({'Timestamp': pd.Timestamp.now(), 'Count': len(detected_objects)}, ignore_index=True)

            # If garbage detected, play alert and send request to boat
            if len(detected_objects) > 0:
                text = "Ada sampah"
                with tempfile.NamedTemporaryFile(delete=False, suffix='.mp3') as temp_audio:
                    temp_audio_path = temp_audio.name
                    tts = gTTS(text=text, lang='id')
                    tts.save(temp_audio_path)
                try:
                    playsound(temp_audio_path)
                except Exception as e:
                    pass

                try:
                    os.remove(temp_audio_path)
                except Exception as e:
                    pass

                try:
                    requests.get(ada_sampah_url)
                    st.info("GET request sent to /ada_sampah")
                except Exception as e:
                    pass

            # Update video feed in Streamlit
            frame_rgb = cv2.cvtColor(im, cv2.COLOR_BGR2RGB)
            video_feed.image(frame_rgb, channels="RGB")

            # Update latest detected objects
            latest_objects = cropped_imgs[-5:]  # Get the latest 5 detected objects
            cols = cropped_images.columns(5)
            for i, col in enumerate(cols):
                if i < len(latest_objects):
                    col.image(latest_objects[i], channels="BGR")
                else:
                    col.empty()

        except Exception as e:
            st.error(e)
            continue

        key = cv2.waitKey(5)
        if key == ord('q'):
            break

    cv2.destroyAllWindows()

# Control buttons
if st.button('Start Detection'):
    st.session_state["detection_running"] = True
    run_detection()

# Logs and alerts
log_placeholder = st.empty()
log_placeholder.text_area('System Logs')

