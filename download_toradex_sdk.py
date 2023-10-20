from selenium import webdriver
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.common.exceptions import TimeoutException
from selenium.webdriver.common.by import By
import os
import time

chromeOptions = webdriver.ChromeOptions()
prefs = {"download.default_directory" : os.getcwd()}
chromeOptions.add_experimental_option("prefs",prefs)

driver = webdriver.Chrome(options=chromeOptions)
#implicit wait
driver.implicitly_wait(0.5)
#maximize browser
driver.maximize_window()
#navigate a page
driver.get("https://docs.toradex.com/102578-toradex-ce8-sdk.zip?v=7")
try:
   w = WebDriverWait(driver, 8)
   w.until(EC.presence_of_element_located((By.ID, "download-frm")))
   print("Page load happened")
   driver.find_element("id", "download-frm").submit()
   print("Submit happened")
   time.sleep(20)
   print("Done")
except TimeoutException:
   print("Timeout happened no page load")
driver.close()
