# 2025-12-07T18:51:53.432392881
import vitis

client = vitis.create_client()
client.set_workspace(path="versal-ai-benchmarking")

comp = client.create_hls_component(name = "hls_component",cfg_file = ["hls_config.cfg"],template = "empty_hls_component")

client.delete_component(name="hls_component")

comp = client.create_aie_component(name="aie_component", platform = "/opt/xilinx/2025.2/Vitis/base_platforms/xilinx_vck190_base_202520_1/xilinx_vck190_base_202520_1.xpfm", template = "empty_aie_component")

client.delete_component(name="aie_component")

comp = client.create_aie_component(name="aie_component", platform = "/opt/xilinx/2025.2/Vitis/base_platforms/xilinx_vck190_base_202520_1/xilinx_vck190_base_202520_1.xpfm", template = "empty_aie_component")

comp = client.get_component(name="aie_component")
status = comp.import_files(from_loc="", files=["/home/dan/Documents/Xilinx/versal-ai-benchmarking/aie_kernel.h", "/home/dan/Documents/Xilinx/versal-ai-benchmarking/graph.cpp", "/home/dan/Documents/Xilinx/versal-ai-benchmarking/graph.h", "/home/dan/Documents/Xilinx/versal-ai-benchmarking/MatMul.cpp"], is_skip_copy_sources = False)

status = comp.update_top_level_file(top_level_file="graph.cpp")

client.delete_component(name="aie_component")

