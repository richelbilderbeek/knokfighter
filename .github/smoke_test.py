"""Check that the WebAssembly game starts in a browser and reacts to keys."""
import base64
import sys
import time

from playwright.sync_api import sync_playwright

URL = "http://localhost:8000/knokfighter.html"

with sync_playwright() as p:
    browser = p.chromium.launch(channel="chrome")
    # The same size as the iframe on the website
    page = browser.new_page(viewport={"width": 852, "height": 658})
    errors = []
    page.on("pageerror", lambda e: errors.append(str(e)))
    page.on("console", lambda m: print("console:", m.text))
    page.on("response", lambda r: print("response:", r.status, r.url))
    page.goto(URL)
    time.sleep(15)
    page.screenshot(path="screenshot_1_start.png")
    start = page.screenshot()
    # Give the game the keyboard focus, then start it
    page.mouse.click(400, 300)
    page.keyboard.press("Space")
    time.sleep(2)
    page.screenshot(path="screenshot_2_running.png")
    running = page.screenshot()
    # Walk left, then kick
    for _ in range(3):
        page.keyboard.press("ArrowLeft")
        time.sleep(0.6)
    page.keyboard.press("w")
    time.sleep(0.3)
    page.screenshot(path="screenshot_3_kick.png")
    kick = page.screenshot()
    browser.close()

# Show the screenshots in the log, as small JPEG images
for name in ["screenshot_1_start", "screenshot_2_running", "screenshot_3_kick"]:
    with open(name + ".png", "rb") as f:
        print(name + ":", base64.b64encode(f.read()).decode())

if errors:
    sys.exit(f"Page errors: {errors}")
if start == running:
    sys.exit("Pressing space did not start the game")
if running == kick:
    sys.exit("Pressing keys did not change the game")
print("OK")
