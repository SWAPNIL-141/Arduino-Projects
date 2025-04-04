import spotipy
from spotipy.oauth2 import SpotifyOAuth
import serial
import time
from dotenv import load_dotenv
import os

load_dotenv()

def connect_serial():
    max_retries = 3
    for attempt in range(max_retries):
        try:
            ser = serial.Serial("COM9", 9600, timeout=1)
            time.sleep(2)
            print("Seri port bağlantısı başarılı")
            return ser
        except serial.SerialException as e:
            print(f"Seri port hatası (Deneme {attempt + 1}/{max_retries}): {e}")
            if attempt == max_retries - 1:
                return None
            time.sleep(2)

def send_to_arduino(ser, message):
    if ser is None:
        ser = connect_serial()
    if ser:
        try:
            ser.write(f"{message}\n".encode('utf-8'))
            ser.flush()
            print(f"Arduino'ya gönderildi: {message}")
            return ser
        except serial.SerialException as e:
            print(f"Gönderim hatası: {e}")
            try:
                ser.close()
            except:
                pass
            return None
    return ser

ser = connect_serial()
if ser:
    ser = send_to_arduino(ser, "Python Started")

try:
    sp = spotipy.Spotify(auth_manager=SpotifyOAuth(
        client_id=os.getenv("SPOTIPY_CLIENT_ID"),
        client_secret=os.getenv("SPOTIPY_CLIENT_SECRET"),
        redirect_uri=os.getenv("SPOTIPY_REDIRECT_URI"),
        scope="user-read-playback-state,user-read-currently-playing"
    ))
    print("Spotify API bağlantısı başarılı")
except Exception as e:
    print(f"Spotify bağlantı hatası: {e}")
    exit(1)

previous_track_id = None
previous_playing_state = None
spotify_connected = False
last_update_time = time.time()

while True:
    try:
        current_time = time.time()
        if current_time - last_update_time > 300:
            sp = spotipy.Spotify(auth_manager=SpotifyOAuth(
                client_id=os.getenv("SPOTIPY_CLIENT_ID"),
                client_secret=os.getenv("SPOTIPY_CLIENT_SECRET"),
                redirect_uri=os.getenv("SPOTIPY_REDIRECT_URI"),
                scope="user-read-playback-state,user-read-currently-playing"
            ))
            last_update_time = current_time

        current_track = sp.current_playback()
        
        if current_track is not None:
            if not spotify_connected:
                ser = send_to_arduino(ser, "Spotify Connected")
                spotify_connected = True
            
            current_playing = current_track['is_playing']
            current_track_id = current_track.get('item', {}).get('id') if current_track.get('item') else None
            
            if current_playing != previous_playing_state:
                if current_playing:
                    ser = send_to_arduino(ser, "Playback Started")
                else:
                    ser = send_to_arduino(ser, "Playback Paused")
                previous_playing_state = current_playing
            
            if current_playing and current_track_id and current_track_id != previous_track_id:
                track_name = current_track['item']['name']
                artist_names = ", ".join([artist['name'] for artist in current_track['item']['artists']])
                song_info = f"{track_name}|{artist_names}"
                ser = send_to_arduino(ser, song_info)
                previous_track_id = current_track_id
            
            elif not current_playing and previous_track_id:
                ser = send_to_arduino(ser, "Playback Paused")
                previous_track_id = None
        
        else:
            if spotify_connected:
                ser = send_to_arduino(ser, "Spotify Disconnected")
                spotify_connected = False
                previous_track_id = None
                previous_playing_state = None
    
    except spotipy.SpotifyException as e:
        print(f"Spotify API hatası: {e}")
        if e.http_status == 401:
            try:
                sp = spotipy.Spotify(auth_manager=SpotifyOAuth(
                    client_id=os.getenv("SPOTIPY_CLIENT_ID"),
                    client_secret=os.getenv("SPOTIPY_CLIENT_SECRET"),
                    redirect_uri=os.getenv("SPOTIPY_REDIRECT_URI"),
                    scope="user-read-playback-state,user-read-currently-playing"
                ))
                print("Token yenilendi")
            except Exception as auth_error:
                print(f"Token yenileme hatası: {auth_error}")
                if spotify_connected:
                    ser = send_to_arduino(ser, "Spotify Disconnected")
                    spotify_connected = False
    
    except Exception as e:
        print(f"Genel hata: {str(e)}")
        if spotify_connected:
            ser = send_to_arduino(ser, "Spotify Disconnected")
            spotify_connected = False
    
    time.sleep(1)
