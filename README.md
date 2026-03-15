# TLF
Open-source implementation of the TLF algorithm proposed in the paper 《Traffic-Aware Design for Multi-dimensional Lookup and Forwarding: From IP Routing to Packet Classification》.

## Packet Classification Test
```bash
cd PacketCls/
make
./main --run_mode Classification --method_name <method_name> --rules_file <rules_file_path> --traces_file <traces_file_path> --output_file <output_log_path> --lookup_round <lookup_rounds> --topk_num <topk_value>
```
For Example:
```bash
cd PacketCls/
make
./main  --run_mode Classification --method_name Auto --rules_file ./Dataset/acl1_100K_2_0.5_0.1 --traces_file ./Dataset/acl1_100K_2_0.5_0.1_trace_1.00 --output_file tmp.log --lookup_round 1 --topk_num 300
```

## Longest Prefix Matching (LPM) Test
```bash
cd LPM/
make
./main --run_mode LPM --method_name <method_name> --prefixs_file <prefix_file_path> --traces_file <traces_file_path> --output_file <output_log_path> --lookup_round <lookup_rounds> --TopK <topk_value>
```

For Example:
```bash
cd LPM/
make
./main  --run_mode LPM --method_name Auto  --prefixs_file ./Dataset/bview.20171001.0800.ip6.prefix.txt  --traces_file ./Dataset/bview.20171001.0800_1_1_10.ip6.traffic.txt  --output_file tmp.log --lookup_round 10 --TopK 5000
```