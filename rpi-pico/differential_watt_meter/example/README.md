# Battery Test Data

Battery measurements done between 2022-09-27 and 2022-10-01. Measuring done with `myzhang1029/codes/rpi-pico/differential_watt_meter`.

Read the TSV files with
```python
data = pd.read_csv(name, sep='\t', header=None, names=["shunt", "shunt_v", "source", "source_v", "bias", "millisecs"])
data["current_a"] = data["shunt_v"]/3.5
```
From my record, the shunt resistance seems to be 3.5 ohms. All batteries were discharged with a constant resistance.
`zn1` and `battery1` used 10 ohms + 3.5 ohms, the other two unknown.
