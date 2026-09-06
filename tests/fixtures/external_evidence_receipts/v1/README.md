# External Evidence Receipt Contract Fixture

These files are synthetic contract-test inputs. They were not produced by an
ESP, were not uploaded to hardware, and establish no runtime or physical fact.
Their `contract_fixture` evidence class and `fixture_only` claims are mandatory.

The fixture exists to prove that the public validator binds four independently
hashed layers: candidate manifest and artifact, deployment receipt, raw serial
capture, and observation assertions. Real program owners should author the
same receipt shapes with `retained_external` evidence and retain the raw capture
bytes beside the receipts.
