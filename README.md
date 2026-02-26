# TLF
Open-source implementation of the TLF algorithm proposed in the paper 《Traffic-Aware Design for Multi-dimensional Lookup and Forwarding: From IP Routing to Packet Classification》.

## Packet Classification Test
```bash
cd PacketCls/
make
./main --run_mode Classification --method_name <method_name> --rules_file <rules_file_path> --traces_file <traces_file_path> --output_file <output_log_path> --lookup_round <lookup_rounds> --topk_num <topk_value>
```

## Longest Prefix Matching (LPM) Test
```bash
cd LPM/
make
./main --run_mode LPM --method_name <method_name> --prefixs_file <prefix_file_path> --traces_file <traces_file_path> --output_file <output_log_path> --lookup_round <lookup_rounds> --TopK <topk_value>
```