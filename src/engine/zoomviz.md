# XaoS Dynamic Programming Algorithm Visualization

This document visualizes the dynamic programming technique used in `src/engine/zoom.cpp` for optimal fractal zooming.

## 1. High-Level Algorithm Flow

```mermaid
flowchart TD
    Start([Start: mkrealloc_table]) --> Init[Initialize & Convert to Fixed-Point]
    Init --> Init1[Convert old positions to fixed-point coordinates<br/>pos array: 0 = begin, size*FPMUL = end]
    Init1 --> Init2[Calculate symmetry axis position if applicable]
    Init2 --> Forward[Forward Pass: Dynamic Programming]

    Forward --> FP1[For each new position i = 0 to size-1]
    FP1 --> FP2[Option 1: Calculate this position fresh<br/>Cost = previous_cost + NEWPRICE]
    FP2 --> FP3[Option 2: Try reusing old positions p within range<br/>Cost = previous_cost + PRICE distance, y]
    FP3 --> FP4[Store best solution in dyndata table<br/>Track with best arrays]
    FP4 --> FP5{More positions?}
    FP5 -->|Yes| FP1
    FP5 -->|No| Backward

    Backward[Backward Pass: Backtracking] --> BP1[Start from best solution for position size-1]
    BP1 --> BP2[Follow 'previous' pointers backward]
    BP2 --> BP3{Decode decision}
    BP3 -->|Fresh| BP4[Mark position as recalculate=1]
    BP3 -->|Reuse| BP5[Copy old position, recalculate=0]
    BP4 --> BP6{More positions?}
    BP5 --> BP6
    BP6 -->|Yes| BP2
    BP6 -->|No| Post

    Post[Post-Processing] --> Post1[Adjust positions for smoothness<br/>newpositions]
    Post1 --> Post2[Apply symmetry optimizations<br/>preparesymmetries]
    Post2 --> End([End])

    style Forward fill:#e1f5ff
    style Backward fill:#fff4e1
    style Post fill:#f0ffe1
```

## 2. Cost Model & Decision Process

```mermaid
graph TB
    subgraph "For Each New Position i"
        NewPos[New Position i at coordinate y]

        NewPos --> Decision{Evaluate Options}

        Decision --> Opt1[Option 1: Calculate Fresh]
        Decision --> Opt2[Option 2: Reuse Old Position p]

        Opt1 --> Cost1[Cost = previous_cost + NEWPRICE<br/>NEWPRICE = 4*step²]

        Opt2 --> Range{Is old position p<br/>within range?}
        Range -->|Yes: y - IRANGE ≤ pos ≤ y + IRANGE| InRange
        Range -->|No: Too far away| Skip[Skip - too expensive]

        InRange --> NoDbl{Would this cause<br/>doubling?}
        NoDbl -->|No: p > last_used| Cost2[Cost = previous_cost + distance²<br/>distance = pos - y]
        NoDbl -->|Yes: Already used p| Skip

        Cost1 --> Compare[Compare All Costs]
        Cost2 --> Compare

        Compare --> Best[Choose minimum cost<br/>Store in dyndata table]
    end

    style Opt1 fill:#ffe6e6
    style Opt2 fill:#e6f3ff
    style Best fill:#e6ffe6
```

## 3. Forward Pass: State Transitions

```mermaid
stateDiagram-v2
    direction LR

    [*] --> Pos_0: Start

    state "Position 0" as Pos_0 {
        [*] --> Fresh0: Calculate fresh<br/>cost = NEWPRICE
        Fresh0 --> Store0: Store in dyndata
    }

    state "Position i" as Pos_i {
        [*] --> GetPrev: Get best solution<br/>from position i-1

        GetPrev --> EvalFresh: Option 1: Fresh<br/>cost += NEWPRICE
        GetPrev --> EvalReuse: Option 2: Reuse

        EvalReuse --> CheckRange: Find old positions<br/>in range ±IRANGE

        CheckRange --> TryOld: For each old pos p
        TryOld --> CalcCost: cost = prev + distance²
        CalcCost --> CheckBest: Is this < best?
        CheckBest --> UpdateBest: Yes - update best
        CheckBest --> TryOld: No - try next

        EvalFresh --> StoreBest
        UpdateBest --> StoreBest: Store best solution
        StoreBest --> UpdateArrays: Update best arrays<br/>for position i
    }

    state "Position size-1" as Pos_n {
        [*] --> Final: Calculate final position
        Final --> FinalBest: Store best overall
        FinalBest --> [*]
    }

    Pos_0 --> Pos_i: i++
    Pos_i --> Pos_i: i++
    Pos_i --> Pos_n: i = size-1
    Pos_n --> [*]: Forward pass complete
```

## 4. Data Structures & Storage Layout

```mermaid
graph TD
    subgraph "Dynamic Programming Tables"
        TmpData[tmpdata buffer] --> Partition[Partition into arrays]

        Partition --> PosArray[pos array<br/>Old positions in fixed-point<br/>size + 2 ints]
        Partition --> Best[best array<br/>Best solution at each old pos<br/>for position i-1]
        Partition --> Best1[best1 array<br/>Best solution at each old pos<br/>for position i]
        Partition --> DynData[dyndata array<br/>All possible solutions<br/>size * DSIZE entries]
    end

    subgraph "Double Buffering"
        Best <--> Best1
        Note1[Swap pointers each iteration:<br/>best1 becomes new best]
    end

    subgraph "DynData Encoding"
        DynData --> Entry[Each entry contains:]
        Entry --> Price[price: total cost]
        Entry --> Prev[previous: pointer to previous solution]

        PrevDecode{Decode previous pointer}
        Prev --> PrevDecode
        PrevDecode -->|>= nosetadd| Fresh[Calculate fresh]
        PrevDecode -->|< nosetadd| Reuse[Reuse old position<br/>p = entry - dyndata >> DSIZES]
    end

    style PosArray fill:#ffe6cc
    style Best fill:#e6ccff
    style Best1 fill:#cce6ff
    style DynData fill:#ffffcc
```

## 5. Backward Pass: Backtracking

```mermaid
flowchart TD
    Start([Start: i = size-1]) --> GetBest[bestdata = best solution<br/>for position size-1]

    GetBest --> Loop{i > 0?}
    Loop -->|Yes| Decode[Decode bestdata.previous pointer]
    Loop -->|No| Done([Complete])

    Decode --> Check{Pointer type?}

    Check -->|>= nosetadd or<br/>>= size<<DSIZES| Fresh
    Check -->|Normal pointer| Reuse

    Fresh[Mark as Fresh Calculation] --> F1[realloc recalculate = 1]
    F1 --> F2[realloc dirty = 1]
    F2 --> F3[Assign calculation slot<br/>realloc plus = lastplus++]
    F3 --> Next

    Reuse[Reuse Old Position] --> R1[Decode old position p from pointer:<br/>p = bestdata - dyndata >> DSIZES]
    R1 --> R2[realloc position = fpos p]
    R2 --> R3[realloc plus = p]
    R3 --> R4[realloc dirty = 0<br/>realloc recalculate = 0]
    R4 --> Next

    Next[Follow chain backward] --> N1[bestdata = bestdata.previous]
    N1 --> N2[i--]
    N2 --> N3[realloc--]
    N3 --> Loop

    style Fresh fill:#ffcccc
    style Reuse fill:#ccffcc
    style Done fill:#ccccff
```

## 6. Range Processing: Three Phases

```mermaid
sequenceDiagram
    participant i-1 as Position i-1<br/>(previous)
    participant i as Position i<br/>(current)
    participant Old as Old Positions<br/>(p values)

    Note over Old: Range for i-1: [ps, pe)<br/>Range for i: [ps1, p)

    rect rgb(255, 230, 230)
        Note over i,Old: Phase 1: First Position (p == ps)
        Old->>i: Can only connect to i-1's "fresh" solution
        i->>i: Calculate cost = fresh_i-1 + distance²
    end

    rect rgb(230, 230, 255)
        Note over i-1,Old: Phase 2: Overlapping Range (ps < p < pe)
        i-1->>i: Can connect to i-1's best[p-1]
        loop For each p in overlap
            i->>i: cost = best[p-1] + distance²
            Note over i: Retroactive optimization:<br/>Check if p-1 should recalculate fresh
        end
    end

    rect rgb(230, 255, 230)
        Note over i,Old: Phase 3: Beyond i-1's Range (p >= pe)
        i-1->>i: Base cost is stable (doesn't change with p)
        loop For each remaining p
            i->>i: cost = stable_base + distance²
        end
    end
```

## 7. Example Walkthrough

Consider a simple example with 5 positions:

```mermaid
graph LR
    subgraph "Old Frame"
        O0[Pos 0<br/>coord: 0.0]
        O1[Pos 1<br/>coord: 0.25]
        O2[Pos 2<br/>coord: 0.5]
        O3[Pos 3<br/>coord: 0.75]
        O4[Pos 4<br/>coord: 1.0]
    end

    subgraph "New Frame (zoomed/panned)"
        N0[Pos 0<br/>coord: 0.1]
        N1[Pos 1<br/>coord: 0.3]
        N2[Pos 2<br/>coord: 0.5]
        N3[Pos 3<br/>coord: 0.7]
        N4[Pos 4<br/>coord: 0.9]
    end

    O2 -.->|Reuse!<br/>distance=0| N2
    O1 -.->|Reuse<br/>small distance| N1
    O3 -.->|Reuse<br/>small distance| N3
    N0 -.->|Calculate fresh<br/>no close match| N0
    N4 -.->|Calculate fresh<br/>no close match| N4
```

## Key Insights

1. **Dynamic Programming Table**: The algorithm builds a table of size `n × DSIZE` where `n` is the number of rows/columns. Each entry stores the minimum cost to reach that state.

2. **Cost Function**:
   - **Reuse cost**: `(old_position - new_position)²` (in fixed-point)
   - **Fresh calculation cost**: `NEWPRICE = (4 × step)²`
   - The algorithm picks whichever is cheaper

3. **No Doubling Constraint**: Each old position can be reused at most once, preventing visual artifacts from duplicating rows/columns.

4. **Retroactive Optimization**: When the algorithm discovers a better path (lines 869-875 in zoom.cpp), it goes back and updates previous decisions. This is a clever optimization that improves solution quality.

5. **Fixed-Point Arithmetic**: All positions are converted to fixed-point integers (`FPMUL = 64`) for faster computation without floating-point overhead.

6. **Symmetry Optimization**: After finding the optimal solution, `preparesymmetries()` exploits fractal symmetries to avoid even more calculations.

## Performance Impact

This algorithm is why XaoS can zoom smoothly in real-time:
- Instead of recalculating all pixels, it reuses ~70-90% of old rows/columns
- The DP overhead is only ~4% of total calculation time (per comments)
- Solid guessing further reduces pixel calculations by detecting uniform regions

## Related Files

- [src/engine/zoom.cpp:658-1120](src/engine/zoom.cpp#L658-L1120) - The `mkrealloc_table()` function
- [src/engine/calculate.h](src/engine/calculate.h) - The expensive `calculate()` function this algorithm tries to avoid
- [docs/algorithms.md](docs/algorithms.md) - Documentation on the approximation algorithm
