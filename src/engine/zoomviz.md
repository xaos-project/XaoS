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

    style Forward stroke:#4a9eff,stroke-width:3px
    style Backward stroke:#ff9a4a,stroke-width:3px
    style Post stroke:#4aff9a,stroke-width:3px
```

## 2. Cost Model & Decision Process

```mermaid
flowchart TD
    NewPos[New Position i at coordinate y]
    NewPos --> Decision{Evaluate Options}

    Decision --> Opt1[Option 1: Calculate Fresh]
    Opt1 --> Cost1[Cost = previous_cost + NEWPRICE<br/>NEWPRICE = 4*step²]
    Cost1 --> Compare[Compare All Costs]

    Decision --> Opt2[Option 2: Reuse Old Position p]
    Opt2 --> Range{Is old position p<br/>within range?}
    Range -->|No: Too far away| Skip1[Skip - too expensive]
    Range -->|Yes: y - IRANGE ≤ pos<br/>≤ y + IRANGE| InRange[Position in Range]

    InRange --> NoDbl{Would this cause<br/>doubling?}
    NoDbl -->|Yes: Already used p| Skip2[Skip - already used]
    NoDbl -->|No: p > last_used| Cost2[Cost = previous_cost + distance²<br/>distance = pos - y]

    Cost2 --> Compare
    Compare --> Best[Choose minimum cost<br/>Store in dyndata table]

    Skip1 --> Compare
    Skip2 --> Compare

    style Opt1 stroke:#ff4a4a,stroke-width:2px
    style Opt2 stroke:#4a9aff,stroke-width:2px
    style Best stroke:#4aff4a,stroke-width:3px
```

## 3. Forward Pass: State Transitions

```mermaid
flowchart TD
    Start([Start]) --> Pos0[Position 0]
    Pos0 --> Fresh0[Calculate fresh<br/>cost = NEWPRICE]
    Fresh0 --> Store0[Store in dyndata]
    Store0 --> PosI[Position i]

    PosI --> GetPrev[Get best solution<br/>from position i-1]
    GetPrev --> Decision{Evaluate Options}

    Decision --> EvalFresh[Option 1: Fresh<br/>cost += NEWPRICE]
    EvalFresh --> StoreBest

    Decision --> EvalReuse[Option 2: Reuse]
    EvalReuse --> CheckRange[Find old positions<br/>in range ±IRANGE]
    CheckRange --> TryLoop[Try each old pos p]
    TryLoop --> CalcCost[cost = prev + distance²]
    CalcCost --> CheckBest{Is this < best?}
    CheckBest -->|Yes| UpdateBest[Update best]
    CheckBest -->|No| TryLoop
    UpdateBest --> StoreBest

    StoreBest[Store best solution] --> UpdateArrays[Update best arrays]
    UpdateArrays --> MorePos{More positions?}
    MorePos -->|Yes: i++| PosI
    MorePos -->|No: Done| Done([Complete])

    style Start stroke:#4a9aff,stroke-width:3px
    style Done stroke:#4aff4a,stroke-width:3px
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

    style PosArray stroke:#ff9a4a,stroke-width:2px
    style Best stroke:#9a4aff,stroke-width:2px
    style Best1 stroke:#4a9aff,stroke-width:2px
    style DynData stroke:#ffff4a,stroke-width:2px
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

    style Fresh stroke:#ff4a4a,stroke-width:3px
    style Reuse stroke:#4aff4a,stroke-width:3px
    style Done stroke:#4a9aff,stroke-width:3px
```

## 6. Range Processing: Three Phases

```mermaid
sequenceDiagram
    participant i-1 as Position i-1<br/>(previous)
    participant i as Position i<br/>(current)
    participant Old as Old Positions<br/>(p values)

    Note over Old: Range for i-1: [ps, pe)<br/>Range for i: [ps1, p)

    Note over i,Old: Phase 1: First Position (p == ps)
    Old->>i: Can only connect to i-1's "fresh" solution
    i->>i: Calculate cost = fresh_i-1 + distance²

    Note over i-1,Old: Phase 2: Overlapping Range (ps < p < pe)
    i-1->>i: Can connect to i-1's best[p-1]
    loop For each p in overlap
        i->>i: cost = best[p-1] + distance²
        Note over i: Retroactive optimization:<br/>Check if p-1 should recalculate fresh
    end

    Note over i,Old: Phase 3: Beyond i-1's Range (p >= pe)
    i-1->>i: Base cost is stable (doesn't change with p)
    loop For each remaining p
        i->>i: cost = stable_base + distance²
    end
```

## 7. Example Walkthrough: Zooming Out

Consider zooming out - the old frame shows a small region, the new frame shows a larger region:

```mermaid
flowchart TD
    subgraph Old["Old Frame: 5 rows at coords [0.2, 0.4, 0.5, 0.6, 0.8]"]
        O0["Row 0 @ 0.2"]
        O1["Row 1 @ 0.4"]
        O2["Row 2 @ 0.5"]
        O3["Row 3 @ 0.6"]
        O4["Row 4 @ 0.8"]
    end

    subgraph New["New Frame: 5 rows at coords [0.0, 0.25, 0.5, 0.75, 1.0]"]
        N0["Row 0 @ 0.0<br/>❌ CALCULATE FRESH<br/>No old row nearby"]
        N1["Row 1 @ 0.25<br/>✓ Reuse O0<br/>distance=0.05"]
        N2["Row 2 @ 0.5<br/>✓ Reuse O2<br/>distance=0.0 EXACT!"]
        N3["Row 3 @ 0.75<br/>✓ Reuse O4<br/>distance=0.05"]
        N4["Row 4 @ 1.0<br/>❌ CALCULATE FRESH<br/>No old row nearby"]
    end

    O0 -.->|distance²=0.0025<br/>cheaper than NEWPRICE| N1
    O2 -.->|distance²=0<br/>perfect reuse!| N2
    O4 -.->|distance²=0.0025<br/>cheaper than NEWPRICE| N3

    style N0 stroke:#ff4a4a,stroke-width:2px
    style N4 stroke:#ff4a4a,stroke-width:2px
    style N1 stroke:#4aff4a,stroke-width:2px
    style N2 stroke:#4aff4a,stroke-width:2px
    style N3 stroke:#4aff4a,stroke-width:2px
```

**Why these decisions?**

1. **Row 0 @ 0.0**: Nearest old row is O0 @ 0.2 (distance = 0.2). Cost to reuse = 0.04, but NEWPRICE ≈ (4×step)² = (4×0.25)² = 1.0. Reuse would be cheaper! But O0 is needed for N1, so we calculate fresh.

2. **Row 1 @ 0.25**: Can reuse O0 @ 0.2 (distance = 0.05, cost = 0.0025). Much cheaper than NEWPRICE = 1.0. **Reuse!**

3. **Row 2 @ 0.5**: Exact match with O2 @ 0.5 (distance = 0, cost = 0). **Perfect reuse!**

4. **Row 3 @ 0.75**: Could reuse O3 @ 0.6 or O4 @ 0.8. O4 is closer (distance = 0.05). But O3 is not reused elsewhere, so we could use either. Algorithm picks O4. **Reuse O4!**

5. **Row 4 @ 1.0**: Nearest is O4 @ 0.8 (distance = 0.2), but O4 is already used by N3. **No doubling allowed!** Must calculate fresh.

**Key insight**: The algorithm prevents "doubling" (reusing the same old row twice) to avoid visual artifacts. Each old row can only be reused once.

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
