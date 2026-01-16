# 2025-12-18T11:47:00.123304669
import vitis

client = vitis.create_client()
client.set_workspace(path="srs-project")

client.delete_component(name="app_component")

vitis.dispose()

